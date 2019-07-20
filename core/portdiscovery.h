#ifndef PORTDISCOVERY_H
#define PORTDISCOVERY_H

#include <functional>
#include <memory>
#include <QList>
#include <QObject>
#include <QList>
#include <QSerialPort>
#include <QTimer>
#include <QtDebug>
#include "machineinfo.h"
#include "serialport.h"
#include "immediatecommands.h"

// A separate class because PortDiscovery is template
class AbstractPortDiscovery : public QObject
{
    Q_OBJECT

public:
    AbstractPortDiscovery()
    {
    }

    virtual std::unique_ptr<SerialPortInterface> obtainPort() = 0;
    virtual void setCharacterSendDelayUs(unsigned long us) = 0;

public slots:
    virtual void start() = 0;

signals:
    void startedDiscoveringPort();
    // After this signal is sent, it is possible to retrieve the open serial port using
    // PortDiscovery::obtainPort(). portDiscoverer is the instance of PortDiscovery that found the
    // port, beware of threading issues. info must not be deleted, but it is guaranteed to be
    // thread-safe. It is valid until a new port is found
    void portFound(MachineInfo* info, AbstractPortDiscovery* portDiscoverer);
};

template <class SerialPortInfo>
class PortDiscovery : public AbstractPortDiscovery
{
public:
    using PortListingFuncT = std::function<QList<SerialPortInfo>()>;
    using SerialPortFactoryT = std::function<std::unique_ptr<SerialPortInterface>(SerialPortInfo)>;

public:
    // scanDelayMillis is how much to wait between two consecutive scans in milliseconds,
    // portReadInterval is for how long to attempt to read from a possibly matching port in
    // milliseconds and maxReadAttemptsPerPort is how many attempts to do before moving to another
    // port. characterSendDelayUs is the value to set on every opened serial port
    PortDiscovery(PortListingFuncT portListingFunc, SerialPortFactoryT serialPortFactory, int scanDelayMillis, int portPollInterval, int maxReadAttemptsPerPort, unsigned long characterSendDelayUs)
        : AbstractPortDiscovery()
        , m_portListingFunc(portListingFunc)
        , m_serialPortFactory(serialPortFactory)
        , m_scanDelayMillis(scanDelayMillis)
        , m_portPollInterval(portPollInterval)
        , m_maxReadAttemptsPerPort(maxReadAttemptsPerPort)
        , m_characterSendDelayUs(characterSendDelayUs)
        , m_currentPortAttempt(0)
        , m_searchingPort(false)
    {
        m_timer.setSingleShot(true);
        connect(&m_timer, &QTimer::timeout, this, &PortDiscovery::timeout);
    }

    // This returns an open serial port for the found machine after the portFound signal is emitted.
    // Port is moved, calls following the first one return a null port
    std::unique_ptr<SerialPortInterface> obtainPort() override
    {
        m_serialPort->disconnect(this);
        return std::move(m_serialPort);
    }

    void setCharacterSendDelayUs(unsigned long us) override
    {
        m_characterSendDelayUs = us;
    }

    void start() override
    {
        emit startedDiscoveringPort();

        searchPort();
    }

private:
    void timeout()
    {
        if (m_serialPort) {
            askFirmwareVersion();
        } else {
            searchPort();
        }
    }

    void serialPortError()
    {
        moveToNextPortOrScheduleRescan();
    }

    void searchPort()
    {
        SearchingPortRAII searchingPortRAII(m_searchingPort);

        rescanIfNeeded();

        bool candidateFound = false;
        while (!m_portsQueue.isEmpty() && !candidateFound) {
            auto p = m_portsQueue.takeFirst();

            if (vendorAndProductMatch(p)) {
                qDebug() << "Found a port with matching vendor and product identifier";
                initializePort(p);

                // m_serialPort might be reset in case of errors
                if (m_serialPort) {
                    askFirmwareVersion();

                    candidateFound = true;
                    break;
                }
            }
        }

        if (!candidateFound) {
            m_timer.start(m_scanDelayMillis);
        }
    }

    void initializePort(const SerialPortInfo& info)
    {
        m_serialPort = m_serialPortFactory(info);
        connect(m_serialPort.get(), &SerialPortInterface::dataAvailable,
                this, &PortDiscovery<SerialPortInfo>::dataAvailable);
        connect(m_serialPort.get(), &SerialPortInterface::errorOccurred,
                this, &PortDiscovery<SerialPortInfo>::serialPortError);

        m_serialPort->setCharacterSendDelayUs(m_characterSendDelayUs);
        m_serialPort->open();

        // m_serialPort might be reset in case of errors
        if (m_serialPort) {
            // This is necessary in case the machine is in error
            m_serialPort->write(QByteArray(1, ImmediateCommands::hardReset));
        }
    }

    void rescanIfNeeded()
    {
        if (m_portsQueue.isEmpty()) {
            m_portsQueue = m_portListingFunc();
        }
    }

    void askFirmwareVersion()
    {
        m_currentPortAttempt++;

        if (m_currentPortAttempt > m_maxReadAttemptsPerPort) {
            // Failure with this port, close and wait
            moveToNextPortOrScheduleRescan();
        } else {
            m_serialPort->write("$I\n");
            m_timer.start(m_portPollInterval);
        }
    }

    void moveToNextPortOrScheduleRescan()
    {
        m_serialPort.reset();
        m_currentPortAttempt = 0;
        m_receivedData.clear();

        if (!m_searchingPort) {
            if (m_portsQueue.isEmpty()) {
                m_timer.start(m_scanDelayMillis);
            } else {
                searchPort();
            }
        }
    }

    bool vendorAndProductMatch(const SerialPortInfo& p)
    {
        return  (p.vendorIdentifier() == 0x2341 && p.productIdentifier() == 0x0043)  // Arduino UNO R3 with ATmega16u2
              ||(p.vendorIdentifier() == 0x10c4 && p.productIdentifier() == 0xea60); // Silabs CP210x
    }

    void dataAvailable()
    {
        m_receivedData += m_serialPort->readAll();
        qDebug() << "Message received from machine:" << m_receivedData;

        auto info = MachineInfo::createFromString(m_receivedData);
        if (info) {
            m_machineInfo = std::move(info);
            emit portFound(m_machineInfo.get(), this);
            m_timer.stop();
        }
    }

    const PortListingFuncT m_portListingFunc;
    const SerialPortFactoryT m_serialPortFactory;
    const int m_scanDelayMillis;
    const int m_portPollInterval;
    const int m_maxReadAttemptsPerPort;
    int m_characterSendDelayUs;
    QTimer m_timer;
    std::unique_ptr<SerialPortInterface> m_serialPort;
    QByteArray m_receivedData;
    int m_currentPortAttempt;
    QList<SerialPortInfo> m_portsQueue;
    bool m_searchingPort;
    std::unique_ptr<MachineInfo> m_machineInfo;

    // Sets searchingPort to true in costructor and to false in destructor
    class SearchingPortRAII {
    public:
        SearchingPortRAII(bool &searchingPort)
            : m_searchingPort(searchingPort)
        {
            m_searchingPort = true;
        }
        ~SearchingPortRAII()
        {
            m_searchingPort = false;
        }
    private:
        bool& m_searchingPort;
    };
};

#endif // PORTDISCOVERY_H

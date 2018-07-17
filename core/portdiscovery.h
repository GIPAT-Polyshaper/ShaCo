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

// A separate class because PortDiscovery is template
class AbstractPortDiscovery : public QObject
{
    Q_OBJECT

public:
    AbstractPortDiscovery()
    {
    }

    virtual std::unique_ptr<SerialPortInterface> obtainPort() = 0;

public slots:
    virtual void start() = 0;

signals:
    void startedDiscoveringPort();
    // After this signal is sent, it is possible to retrieve the open serial port using
    // PortDiscovery::obtainPort(). portDiscoverer is the instance of PortDiscovery that found the
    // port, beware of threading issues
    void portFound(MachineInfo info, AbstractPortDiscovery* portDiscoverer);
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
    // port
    PortDiscovery(PortListingFuncT portListingFunc, SerialPortFactoryT serialPortFactory, int scanDelayMillis, int portPollInterval, int maxReadAttemptsPerPort)
        : AbstractPortDiscovery()
        , m_portListingFunc(portListingFunc)
        , m_serialPortFactory(serialPortFactory)
        , m_scanDelayMillis(scanDelayMillis)
        , m_portPollInterval(portPollInterval)
        , m_maxReadAttemptsPerPort(maxReadAttemptsPerPort)
        , m_currentPortAttempt(0)
    {
        m_timer.setSingleShot(true);
        connect(&m_timer, &QTimer::timeout, this, &PortDiscovery::timeout);
    }

    void start() override
    {
        emit startedDiscoveringPort();

        searchPort();
    }

    // This returns an open serial port for the found machine after the portFound signal is emitted.
    // Port is moved, calls following the first one return a null port
    std::unique_ptr<SerialPortInterface> obtainPort() override
    {
        m_serialPort->disconnect(this);
        return std::move(m_serialPort);
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

    void searchPort()
    {
        rescanIfNeeded();

        bool candidateFound = false;
        while (!m_portsQueue.isEmpty() && !candidateFound) {
            auto p = m_portsQueue.takeFirst();

            if (vendorAndProductMatch(p)) {
                qDebug() << "Found a port with matching vendor and product identifier";
                m_serialPort = m_serialPortFactory(p);
                connect(m_serialPort.get(), &SerialPortInterface::dataAvailable,
                        this, &PortDiscovery<SerialPortInfo>::dataAvailable);
                m_serialPort->open();
                askFirmwareVersion();

                candidateFound = true;
                break;
            }
        }

        if (!candidateFound) {
            m_timer.start(m_scanDelayMillis);
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

        if (m_portsQueue.isEmpty()) {
            m_timer.start(m_scanDelayMillis);
        } else {
            searchPort();
        }
    }

    bool vendorAndProductMatch(const SerialPortInfo& p)
    {
        return p.vendorIdentifier() == 0x2341 && p.productIdentifier() == 0x0043;
    }

    void dataAvailable()
    {
        m_receivedData += m_serialPort->readAll();
        qDebug() << "Message received from machine:" << m_receivedData;

        auto info = MachineInfo::createFromString(m_receivedData);
        if (info.isValid()) {
            emit portFound(info, this);
            m_timer.stop();
        }
    }

    const PortListingFuncT m_portListingFunc;
    const SerialPortFactoryT m_serialPortFactory;
    const int m_scanDelayMillis;
    const int m_portPollInterval;
    const int m_maxReadAttemptsPerPort;
    QTimer m_timer;
    std::unique_ptr<SerialPortInterface> m_serialPort;
    QByteArray m_receivedData;
    int m_currentPortAttempt;
    QList<SerialPortInfo> m_portsQueue;
};

#endif // PORTDISCOVERY_H

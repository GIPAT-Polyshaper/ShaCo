#ifndef PORTDISCOVERY_H
#define PORTDISCOVERY_H

#include <functional>
#include <memory>
#include <QList>
#include <QObject>
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
    PortDiscovery(PortListingFuncT portListingFunc, SerialPortFactoryT serialPortFactory, int scanDelayMillis, int maxReadAttemptsPerPort)
        : AbstractPortDiscovery()
        , m_portListingFunc(portListingFunc)
        , m_serialPortFactory(serialPortFactory)
        , m_scanDelayMillis(scanDelayMillis)
        , m_maxReadAttemptsPerPort(maxReadAttemptsPerPort)
        , m_timer(new QTimer(this))
        , m_serialPort()
    {
        m_timer->setSingleShot(true);
        connect(m_timer, &QTimer::timeout, this, &PortDiscovery::searchPort);
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
        return std::move(m_serialPort);
    }

private:
    void searchPort()
    {
        bool scheduleNextScan = true;

        for (const auto& p: m_portListingFunc()) {
            if (vendorAndProductMatch(p)) {
                qDebug() << "Found a port with matching vendor and product identifier";
                auto serialPort = m_serialPortFactory(p);
                auto machine = getMachineInfo(serialPort.get());

                if (machine.isValid()) {
                    m_serialPort = std::move(serialPort);
                    emit portFound(machine, this);
                    scheduleNextScan = false;
                    break;
                }
            }
        }

        if (scheduleNextScan) {
            m_timer->start(m_scanDelayMillis);
        }
    }

    bool vendorAndProductMatch(const SerialPortInfo& p)
    {
        return p.vendorIdentifier() == 0x2341 && p.productIdentifier() == 0x0043;
    }

    MachineInfo getMachineInfo(SerialPortInterface* p)
    {
        QByteArray answer;
        p->open(QIODevice::ReadWrite, QSerialPort::Baud115200);
        p->write("$I\n");

        for (int i = 0; i < m_maxReadAttemptsPerPort && !answer.endsWith("ok\r\n"); ++i) {
            auto partial = p->read(100, 1000);
            answer.append(partial);
        }

        qDebug() << "machine answer: " << answer;
        return MachineInfo::createFromString(answer);
    }

    const PortListingFuncT m_portListingFunc;
    const SerialPortFactoryT m_serialPortFactory;
    const int m_scanDelayMillis;
    const int m_maxReadAttemptsPerPort;
    QTimer* const m_timer;
    std::unique_ptr<SerialPortInterface> m_serialPort;
};

#endif // PORTDISCOVERY_H

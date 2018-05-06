#include "controller.h"
#include <memory>
#include <QMetaObject>
#include <QSerialPortInfo>
#include "core/portdiscovery.h"

Controller::Controller(QObject *parent)
    : QObject(parent)
    , m_portThread()
{
    auto portDiscovery = new PortDiscovery<QSerialPortInfo>(
                QSerialPortInfo::availablePorts,
                [](QSerialPortInfo p){ return std::make_unique<SerialPort>(p); },
                1000,
                100);
    portDiscovery->moveToThread(&m_portThread);
    connect(&m_portThread, &QThread::finished, portDiscovery, &QObject::deleteLater);

    connect(portDiscovery, &PortDiscovery<QSerialPortInfo>::startedDiscoveringPort, this, &Controller::startedPortDiscovery);
    connect(portDiscovery, &PortDiscovery<QSerialPortInfo>::portFound, this, &Controller::signalPortFound);

    m_portThread.start();
    QMetaObject::invokeMethod(portDiscovery, &PortDiscovery<QSerialPortInfo>::start);
}

Controller::~Controller()
{
    m_portThread.quit();
    m_portThread.wait();
}

void Controller::signalPortFound(MachineInfo info)
{
    emit portFound(info.machineName(), info.firmwareVersion());
}

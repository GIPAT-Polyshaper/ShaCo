#include "controller.h"
#include <memory>
#include <QMetaObject>

Controller::Controller(QObject *parent)
    : QObject(parent)
    , m_portThread()
    , m_connected(false)
    , m_portDiscoverer(new PortDiscovery<QSerialPortInfo>(QSerialPortInfo::availablePorts, [](QSerialPortInfo p){ return std::make_unique<SerialPort>(p); }, 100, 100))
    , m_machineCommunicator(new MachineCommunication())
{
    moveToPortThread(m_portDiscoverer);
    moveToPortThread(m_machineCommunicator);

    connect(m_portDiscoverer, &PortDiscovery<QSerialPortInfo>::startedDiscoveringPort, this, &Controller::startedPortDiscovery);
    connect(m_portDiscoverer, &PortDiscovery<QSerialPortInfo>::portFound, this, &Controller::signalPortFound);
    connect(m_portDiscoverer, &PortDiscovery<QSerialPortInfo>::portFound, m_machineCommunicator, &MachineCommunication::portFound);
    connect(m_machineCommunicator, &MachineCommunication::dataSent, this, &Controller::dataSent);
    connect(m_machineCommunicator, &MachineCommunication::dataReceived, this, &Controller::dataReceived);

    m_portThread.start();
    QMetaObject::invokeMethod(m_portDiscoverer, "start");
}

Controller::~Controller()
{
    m_portThread.quit();
    m_portThread.wait();
}

bool Controller::connected() const
{
    return m_connected;
}

void Controller::sendLine(QByteArray line)
{
    QMetaObject::invokeMethod(m_machineCommunicator, "writeLine", Q_ARG(QByteArray, line));
}

void Controller::signalPortFound(MachineInfo info)
{
    m_connected = true;

    emit portFound(info.machineName(), info.firmwareVersion());
    emit connectedChanged();
}

void Controller::moveToPortThread(QObject* obj)
{
    obj->moveToThread(&m_portThread);
    connect(&m_portThread, &QThread::finished, obj, &QObject::deleteLater);
}

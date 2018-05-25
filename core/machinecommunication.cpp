#include "machinecommunication.h"

MachineCommunication::MachineCommunication()
    : QObject()
    , m_serialPort()
{
}

void MachineCommunication::portFound(MachineInfo, AbstractPortDiscovery* portDiscoverer)
{
    m_serialPort = portDiscoverer->obtainPort();

    connect(m_serialPort.get(), &SerialPortInterface::dataAvailable, this, &MachineCommunication::readData);
}

void MachineCommunication::writeLine(QByteArray data)
{
    m_serialPort->write(data);
    m_serialPort->write("\n");

    emit dataSent(data + "\n");
}

void MachineCommunication::readData()
{
    emit dataReceived(m_serialPort->readAll());
}

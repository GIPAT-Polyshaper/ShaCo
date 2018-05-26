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

    if (!checkPortInErrorAndCloseIfTrue()) {
        emit dataSent(data + "\n");
    }
}

void MachineCommunication::readData()
{
    auto data = m_serialPort->readAll();

    if (!checkPortInErrorAndCloseIfTrue()) {
        emit dataReceived(data);
    }
}

bool MachineCommunication::checkPortInErrorAndCloseIfTrue()
{
    if (m_serialPort->inError()) {
        emit portClosed(m_serialPort->errorString());
        m_serialPort.reset();

        return true;
    } else {
        return false;
    }
}

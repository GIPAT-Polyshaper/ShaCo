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

    emit portOpened();
}

void MachineCommunication::writeData(QByteArray data)
{
    if (!m_serialPort) {
        return;
    }

    m_serialPort->write(data);

    if (!checkPortInErrorAndCloseIfTrue()) {
        emit dataSent(data);
    }
}

void MachineCommunication::writeLine(QByteArray data)
{
    writeData(data + "\n");
}

void MachineCommunication::closePortWithError(QString reason)
{
    emit portClosedWithError(reason);
    m_serialPort.reset();
}

void MachineCommunication::closePort()
{
    emit portClosed();
    m_serialPort.reset();
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
        emit portClosedWithError(m_serialPort->errorString());
        m_serialPort.reset();

        return true;
    } else {
        return false;
    }
}

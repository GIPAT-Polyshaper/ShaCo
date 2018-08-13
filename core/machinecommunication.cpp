#include "machinecommunication.h"
#include <QThread>
#include "immediatecommands.h"

MachineCommunication::MachineCommunication(unsigned int hardResetDelay)
    : QObject()
    , m_hardResetDelay(hardResetDelay)
    , m_serialPort()
{
}

void MachineCommunication::portFound(MachineInfo*, AbstractPortDiscovery* portDiscoverer)
{
    m_serialPort = portDiscoverer->obtainPort();

    connect(m_serialPort.get(), &SerialPortInterface::dataAvailable, this, &MachineCommunication::readData);
    connect(m_serialPort.get(), &SerialPortInterface::errorOccurred, this, &MachineCommunication::errorOccurred);

    emit machineInitialized();
}

void MachineCommunication::writeData(QByteArray data)
{
    if (!m_serialPort) {
        return;
    }

    auto res = m_serialPort->write(data);

    if (res != -1) {
        emit dataSent(data);
    }
}

void MachineCommunication::writeLine(QByteArray data)
{
    writeData(data + "\n");
}

void MachineCommunication::closePortWithError(QString reason)
{
    if (m_serialPort) {
        emit portClosedWithError(reason);
        m_serialPort.reset();
    }
}

void MachineCommunication::closePort()
{
    if (m_serialPort) {
        emit portClosed();
        m_serialPort.reset();
    }
}

void MachineCommunication::feedHold()
{
    writeData(QByteArray(1, ImmediateCommands::feedHold));
}

void MachineCommunication::resumeFeedHold()
{
    writeData(QByteArray(1, ImmediateCommands::resumeFeedHold));
}

void MachineCommunication::softReset()
{
    writeData(QByteArray(1, ImmediateCommands::softReset));
}

void MachineCommunication::hardReset()
{
    writeData(QByteArray(1, ImmediateCommands::hardReset));

    // This is needed to give the machine time to start
    QThread::msleep(m_hardResetDelay);

    emit machineInitialized();
}

void MachineCommunication::setCharacterSendDelayUs(unsigned long us)
{
    if (m_serialPort) {
        m_serialPort->setCharacterSendDelayUs(us);
    }
}

void MachineCommunication::readData()
{
    auto data = m_serialPort->readAll();
    m_messageBuffer += data;

    if (!data.isEmpty()) {
        emit dataReceived(data);

        for (auto message: extractMessages()) {
            emit messageReceived(message);
        }
    }
}

void MachineCommunication::errorOccurred()
{
    closePortWithError(m_serialPort->errorString());
}

QList<QByteArray> MachineCommunication::extractMessages()
{
    QList<QByteArray> list;

    int endline;
    while ((endline = m_messageBuffer.indexOf("\r\n")) != -1) {
        list.append(m_messageBuffer.left(endline));
        m_messageBuffer.remove(0, endline + 2);
    }

    return list;
}

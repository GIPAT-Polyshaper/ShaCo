#include "machinecommunication.h"
#include <QThread>

namespace {
    const char feedHoldCommand = '!';
    const char resumeFeedHoldCommand = '~';
    const char softResetCommand = 0x18;
    const char hardResetCommand = 0xC0;
}

MachineCommunication::MachineCommunication(int hardResetDelay)
    : QObject()
    , m_hardResetDelay(hardResetDelay)
    , m_serialPort()
{
}

void MachineCommunication::portFound(MachineInfo, AbstractPortDiscovery* portDiscoverer)
{
    m_serialPort = portDiscoverer->obtainPort();

    connect(m_serialPort.get(), &SerialPortInterface::dataAvailable, this, &MachineCommunication::readData);

    emit machineInitialized();
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

void MachineCommunication::feedHold()
{
    writeData(QByteArray(1, feedHoldCommand));
}

void MachineCommunication::resumeFeedHold()
{
    writeData(QByteArray(1, resumeFeedHoldCommand));
}

void MachineCommunication::softReset()
{
    writeData(QByteArray(1, softResetCommand));
}

void MachineCommunication::hardReset()
{
    writeData(QByteArray(1, hardResetCommand));

    // This is needed to give the machine time to start
    QThread::msleep(m_hardResetDelay);

    emit machineInitialized();
}

void MachineCommunication::readData()
{
    auto data = m_serialPort->readAll();
    m_messageBuffer += data;

    if (!checkPortInErrorAndCloseIfTrue()) {
        emit dataReceived(data);

        for (auto message: extractMessages()) {
            emit messageReceived(message);
        }
    }
}

bool MachineCommunication::checkPortInErrorAndCloseIfTrue()
{
    if (m_serialPort->inError()) {
        closePortWithError(m_serialPort->errorString());

        return true;
    } else {
        return false;
    }
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

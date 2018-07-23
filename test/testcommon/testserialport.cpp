#include "testserialport.h"

TestSerialPort::TestSerialPort()
    : SerialPortInterface()
    , m_inError(false)
{
}

bool TestSerialPort::open()
{
    emit portOpened();
    return true;
}

qint64 TestSerialPort::write(const QByteArray &data)
{
    if (m_inError) {
        return -1;
    }

    m_writtenData += data;

    return data.size();
}

QByteArray TestSerialPort::readAll()
{
    if (m_inError) {
        return QByteArray();
    }

    return m_readData;
}

QString TestSerialPort::errorString() const
{
    return "An error!!! ohoh";
}

void TestSerialPort::close()
{
    emit portClosed();
}

QByteArray TestSerialPort::writtenData() const
{
    return m_writtenData;
}

void TestSerialPort::simulateReceivedData(QByteArray data)
{
    m_readData = data;

    emit dataAvailable();
}

void TestSerialPort::setInError(bool inError)
{
    m_inError = inError;
}

void TestSerialPort::emitErrorSignal()
{
    emit errorOccurred();
}

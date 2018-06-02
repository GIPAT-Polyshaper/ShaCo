#include "testserialport.h"

TestSerialPort::TestSerialPort()
    : SerialPortInterface()
    , m_inError(false)
{
}

bool TestSerialPort::open(QIODevice::OpenMode, qint32)
{
    return true;
}

qint64 TestSerialPort::write(const QByteArray &data)
{
    m_writtenData += data;

    return data.size();
}

QByteArray TestSerialPort::read(int, int) // Not used in this test
{
    throw QString("read should not be used in this test!!!");
    return QByteArray();
}

QByteArray TestSerialPort::readAll()
{
    return m_readData;
}

bool TestSerialPort::inError() const
{
    return m_inError;
}

QString TestSerialPort::errorString() const
{
    return "An error!!! ohoh";
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

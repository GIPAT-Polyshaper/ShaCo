#include "serialport.h"
#include <QtDebug>
#include <QThread>

SerialPortInterface::SerialPortInterface()
    : QObject()
{
}

SerialPort::SerialPort(const QSerialPortInfo& portInfo)
    : SerialPortInterface()
    , m_serialPort(portInfo)
{
    connect(&m_serialPort, &QSerialPort::readyRead, this, &SerialPort::dataAvailable);
}

bool SerialPort::open(QIODevice::OpenMode mode, qint32 baudRate)
{
    m_serialPort.setBaudRate(baudRate);
    m_serialPort.setFlowControl(QSerialPort::HardwareControl);

    auto retval = m_serialPort.open(mode);

    // Reading and discarding initial data. We have to give Arduino some time to boot (board is
    // reset when port is opened)
    QThread::msleep(1500);
    m_serialPort.waitForReadyRead(1500);
    m_serialPort.readAll();

    return retval;
}

qint64 SerialPort::write(const QByteArray& data)
{
    return m_serialPort.write(data);
}

QByteArray SerialPort::read(int msec, int maxBytes)
{
    if (m_serialPort.waitForReadyRead(msec)) {
        return m_serialPort.read(maxBytes);
    }

    return QByteArray();
}

QByteArray SerialPort::readAll()
{
    return m_serialPort.readAll();
}

bool SerialPort::inError() const
{
    return m_serialPort.error() != QSerialPort::NoError;
}

QString SerialPort::errorString() const
{
    return m_serialPort.errorString();
}
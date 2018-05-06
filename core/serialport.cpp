#include "serialport.h"

SerialPortInterface::SerialPortInterface()
    : QObject()
{
}

SerialPort::SerialPort(const QSerialPortInfo& portInfo)
    : SerialPortInterface()
    , m_serialPort(portInfo)
{
}

bool SerialPort::open(QIODevice::OpenMode mode, qint32 baudRate)
{
    m_serialPort.setBaudRate(baudRate);
    m_serialPort.setFlowControl(QSerialPort::HardwareControl);
    return m_serialPort.open(mode);
}

qint64 SerialPort::write(const QByteArray& data)
{
    return m_serialPort.write(data);
}

QByteArray SerialPort::read(int msec)
{
    if (m_serialPort.waitForReadyRead(msec)) {
        return m_serialPort.read(100000); // TODO-TOMMY QUI NON VALORE HARDCODATO.
    }

    return QByteArray();
}

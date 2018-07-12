#include "serialport.h"
#include <QtDebug>
#include <QThread>

namespace {
    const unsigned long charSendDelayMs = 1;
}

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

bool SerialPort::open()
{
    m_serialPort.setBaudRate(QSerialPort::Baud115200);
    m_serialPort.setFlowControl(QSerialPort::HardwareControl);

    auto retval = m_serialPort.open(QIODevice::ReadWrite);

    return retval;
}

qint64 SerialPort::write(const QByteArray& data)
{
    // Suggestion taken from GrblController (https://github.com/zapmaker/GrblHoming/blob/master/rs232.cpp
    // at row 180): "On very fast PCs running Windows we have to slow down the sending of bytes to grbl
    // because grbl loses bytes due to its interrupt service routine (ISR) taking too many clock
    // cycles away from serial handling.". Here we send one byte at a time with an small sleep
    qint64 retval = 0;
    for (int i = 0; i < data.size(); ++i) {
        auto r = m_serialPort.write(&(data.data()[i]), 1);

        if (r == 0) {
            break;
        } else if (r == -1) {
            retval = -1;
            break;
        } else {
            ++retval;
        }

        QThread::msleep(charSendDelayMs);
    }

    return retval;
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

void SerialPort::close()
{
    m_serialPort.close();
}

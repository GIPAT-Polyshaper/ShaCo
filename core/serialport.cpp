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
    , m_characterSendDelayUs(0)
{
    connect(&m_serialPort, &QSerialPort::readyRead, this, &SerialPort::dataAvailable);
    connect(&m_serialPort, &QSerialPort::errorOccurred, this, &SerialPort::signalErrorOccurred);
}

bool SerialPort::open()
{
    m_serialPort.setBaudRate(QSerialPort::Baud115200);
    m_serialPort.setFlowControl(QSerialPort::SoftwareControl);

    auto retval = m_serialPort.open(QIODevice::ReadWrite);

    // required to correctly reset ESP32
    // see ESP32 Datasheet V3.1 pages 9 and 10
    m_serialPort.setRequestToSend(true);
    m_serialPort.setDataTerminalReady(false);
    QThread::usleep(100);
    m_serialPort.setRequestToSend(false);

    return retval;
}

qint64 SerialPort::write(const QByteArray& data)
{
    // Suggestion taken from GrblController (https://github.com/zapmaker/GrblHoming/blob/master/rs232.cpp
    // at row 180): "On very fast PCs running Windows we have to slow down the sending of bytes to grbl
    // because grbl loses bytes due to its interrupt service routine (ISR) taking too many clock
    // cycles away from serial handling.". Here we send one byte at a time with an small delay
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

        if (m_characterSendDelayUs != 0) {
            QThread::usleep(m_characterSendDelayUs);
        }
    }

    return retval;
}

QByteArray SerialPort::readAll()
{
    return m_serialPort.readAll();
}

QString SerialPort::errorString() const
{
    return m_serialPort.errorString();
}

void SerialPort::close()
{
    m_serialPort.close();
}

void SerialPort::setCharacterSendDelayUs(unsigned long us)
{
    m_characterSendDelayUs = us;
}

unsigned long SerialPort::characterSendDelayUs() const
{
    return m_characterSendDelayUs;
}

void SerialPort::signalErrorOccurred(QSerialPort::SerialPortError error)
{
    // We only want to signal error if there was a real error (it seems the signal is emitted also
    // when no error occurs)
    if (error != QSerialPort::SerialPortError::NoError) {
        emit errorOccurred();
    }
}

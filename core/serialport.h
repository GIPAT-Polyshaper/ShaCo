#ifndef SERIALPORT_H
#define SERIALPORT_H

#include <QObject>
#include <QByteArray>
#include <QIODevice>
#include <QSerialPort>
#include <QSerialPortInfo>

class SerialPortInterface : public QObject
{
    Q_OBJECT

public:
    // Instances are inside unique_ptr, parent must be nullptr
    SerialPortInterface();

    virtual bool open(QIODevice::OpenMode mode, qint32 baudRate) = 0;
    virtual qint64 write(const QByteArray& data) = 0;
    virtual QByteArray read(int msec) = 0; // waits at most msec milliseconds
};

class SerialPort : public SerialPortInterface
{
    Q_OBJECT

public:
    SerialPort(const QSerialPortInfo& portInfo);

    bool open(QIODevice::OpenMode mode, qint32 baudRate) override;
    qint64 write(const QByteArray& data) override;
    QByteArray read(int msec) override;

private:
    QSerialPort m_serialPort;
};

#endif // SERIALPORT_H

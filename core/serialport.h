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

    virtual bool open() = 0;
    virtual qint64 write(const QByteArray& data) = 0;
    virtual QByteArray read(int msec, int maxBytes) = 0; // waits at most msec milliseconds
    virtual QByteArray readAll() = 0;
    virtual bool inError() const = 0;
    virtual QString errorString() const = 0;
    virtual void close() = 0;

signals:
    void dataAvailable();
};

class SerialPort : public SerialPortInterface
{
    Q_OBJECT

public:
    SerialPort(const QSerialPortInfo& portInfo);

    bool open() override;
    qint64 write(const QByteArray& data) override;
    QByteArray read(int msec, int maxBytes) override;
    QByteArray readAll() override;
    bool inError() const override;
    QString errorString() const override;
    void close() override;

private:
    QSerialPort m_serialPort;
};

#endif // SERIALPORT_H

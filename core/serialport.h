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
    virtual QByteArray readAll() = 0;
    virtual QString errorString() const = 0;
    virtual void close() = 0;
    virtual void setCharacterSendDelayUs(unsigned long us) = 0;
    virtual unsigned long characterSendDelayUs() const = 0;

signals:
    void dataAvailable();
    void errorOccurred();
};

class SerialPort : public SerialPortInterface
{
    Q_OBJECT

public:
    SerialPort(const QSerialPortInfo& portInfo);

    bool open() override;
    qint64 write(const QByteArray& data) override;
    QByteArray readAll() override;
    QString errorString() const override;
    void close() override;
    void setCharacterSendDelayUs(unsigned long us) override;
    unsigned long characterSendDelayUs() const override;

private:
    void signalErrorOccurred(QSerialPort::SerialPortError error);

private:
    QSerialPort m_serialPort;
    unsigned long m_characterSendDelayUs;
};

#endif // SERIALPORT_H

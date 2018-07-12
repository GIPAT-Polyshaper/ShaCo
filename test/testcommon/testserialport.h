#ifndef TESTSERIALPORT_H
#define TESTSERIALPORT_H

#include "core/serialport.h"

class TestSerialPort : public SerialPortInterface {
    Q_OBJECT

public:
    TestSerialPort();

    bool open() override;
    qint64 write(const QByteArray &data) override;
    QByteArray readAll() override;
    bool inError() const override;
    QString errorString() const override;
    void close() override;
    QByteArray writtenData() const;
    void simulateReceivedData(QByteArray data);
    void setInError(bool inError);

signals:
    void portOpened();
    void portClosed();

private:
    bool m_inError;
    QByteArray m_writtenData;
    QByteArray m_readData;
};

#endif // TESTSERIALPORT_H

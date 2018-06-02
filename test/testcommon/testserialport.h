#ifndef TESTSERIALPORT_H
#define TESTSERIALPORT_H

#include "core/serialport.h"

class TestSerialPort : public SerialPortInterface {
    Q_OBJECT

public:
    TestSerialPort();

    bool open(QIODevice::OpenMode, qint32) override;
    qint64 write(const QByteArray &data) override;
    QByteArray read(int, int) override; // Not used in this test
    QByteArray readAll() override;
    bool inError() const override;
    QString errorString() const override;
    QByteArray writtenData() const;
    void simulateReceivedData(QByteArray data);
    void setInError(bool inError);

private:
    bool m_inError;
    QByteArray m_writtenData;
    QByteArray m_readData;
};

#endif // TESTSERIALPORT_H

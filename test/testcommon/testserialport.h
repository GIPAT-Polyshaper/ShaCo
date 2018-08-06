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
    QString errorString() const override;
    void close() override;
    void setCharacterSendDelayUs(unsigned long us) override;
    unsigned long characterSendDelayUs() const override;
    QByteArray writtenData() const;
    void simulateReceivedData(QByteArray data);
    void setInError(bool inError); // Use this to have write return -1 and readAll an empty array
    void emitErrorSignal();

signals:
    void portOpened();
    void portClosed();

private:
    bool m_inError;
    QByteArray m_writtenData;
    QByteArray m_readData;
    unsigned long m_characterSendDelayUs;
};

#endif // TESTSERIALPORT_H

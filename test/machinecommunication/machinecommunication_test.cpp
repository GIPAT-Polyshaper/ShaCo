#include <memory>
#include <QString>
#include <QtTest>
#include <QCoreApplication>
#include <QSignalSpy>
#include "core/machinecommunication.h"
#include "core/portdiscovery.h"
#include "core/serialport.h"

class TestPortDiscovery : public AbstractPortDiscovery {
    Q_OBJECT

public:
    TestPortDiscovery(SerialPortInterface* serialPort)
        : m_serialPort(serialPort)
    {
    }

    std::unique_ptr<SerialPortInterface> obtainPort() override
    {
        emit serialPortMoved();

        return std::move(m_serialPort);
    }

    void start() override
    {
    }

signals:
    void serialPortMoved();

private:
    std::unique_ptr<SerialPortInterface> m_serialPort;
};

class TestSerialPort : public SerialPortInterface {
    Q_OBJECT

public:
    TestSerialPort()
        : SerialPortInterface()
        , m_inError(false)
    {}

    bool open(QIODevice::OpenMode, qint32) override
    {
        return true;
    }

    qint64 write(const QByteArray &data) override
    {
        m_writtenData += data;

        return data.size();
    }

    QByteArray read(int, int) override // Not used in this test
    {
        throw QString("read should not be used in this test!!!");
        return QByteArray();
    }

    QByteArray readAll() override
    {
        return m_readData;
    }

    bool inError() const override
    {
        return m_inError;
    }

    QString errorString() const override
    {
        return "An error!!! ohoh";
    }

    QByteArray writtenData() const
    {
        return m_writtenData;
    }

    void simulateReceivedData(QByteArray data)
    {
        m_readData = data;

        emit dataAvailable();
    }

    void setInError(bool inError)
    {
        m_inError = inError;
    }

private:
    bool m_inError;
    QByteArray m_writtenData;
    QByteArray m_readData;
};

class MachineCommunicationTest : public QObject
{
    Q_OBJECT

public:
    MachineCommunicationTest();

private Q_SLOTS:
    void grabPortFromPortDiscovererWhenMachineFoundIsCalled();
    void writeDataToSerialPort();
    void emitSignalWhenDataIsWritten();
    void emitSignalWhenDataIsReceived();
    void ifPortIsInErrorAfterWriteClosePortAndEmitSignal();
    void ifPortIsInErrorAfterReadClosePortAndEmitSignal();
};

MachineCommunicationTest::MachineCommunicationTest()
{
}

void MachineCommunicationTest::grabPortFromPortDiscovererWhenMachineFoundIsCalled()
{
    auto serialPort = new TestSerialPort();
    TestPortDiscovery portDiscoverer(serialPort);

    MachineCommunication communicator;

    QSignalSpy spy(&portDiscoverer, &TestPortDiscovery::serialPortMoved);

    communicator.portFound(MachineInfo("a", "1"), &portDiscoverer);

    QCOMPARE(spy.count(), 1);
}

void MachineCommunicationTest::writeDataToSerialPort()
{
    auto serialPort = new TestSerialPort();
    TestPortDiscovery portDiscoverer(serialPort);

    MachineCommunication communicator;

    communicator.portFound(MachineInfo("a", "1"), &portDiscoverer);
    communicator.writeLine("some data to write");

    QCOMPARE(serialPort->writtenData(), "some data to write\n");
}

void MachineCommunicationTest::emitSignalWhenDataIsWritten()
{
    auto serialPort = new TestSerialPort();
    TestPortDiscovery portDiscoverer(serialPort);

    MachineCommunication communicator;

    QSignalSpy spy(&communicator, &MachineCommunication::dataSent);

    communicator.portFound(MachineInfo("a", "1"), &portDiscoverer);
    communicator.writeLine("some data to write");

    QCOMPARE(spy.count(), 1);
    auto data = spy.at(0).at(0).toByteArray();
    QCOMPARE(data, "some data to write\n");
}

void MachineCommunicationTest::emitSignalWhenDataIsReceived()
{
    auto serialPort = new TestSerialPort();
    TestPortDiscovery portDiscoverer(serialPort);

    MachineCommunication communicator;

    QSignalSpy spy(&communicator, &MachineCommunication::dataReceived);

    communicator.portFound(MachineInfo("a", "1"), &portDiscoverer);
    serialPort->simulateReceivedData("Toc toc...");

    QCOMPARE(spy.count(), 1);
    auto data = spy.at(0).at(0).toByteArray();
    QCOMPARE(data, "Toc toc...");
}

void MachineCommunicationTest::ifPortIsInErrorAfterWriteClosePortAndEmitSignal()
{
    auto serialPort = new TestSerialPort();
    TestPortDiscovery portDiscoverer(serialPort);
    serialPort->setInError(true);

    MachineCommunication communicator;

    QSignalSpy spyPortDeleted(serialPort, &QObject::destroyed);
    QSignalSpy spyPortClosed(&communicator, &MachineCommunication::portClosed);
    QSignalSpy spyDataSent(&communicator, &MachineCommunication::dataSent);

    communicator.portFound(MachineInfo("a", "1"), &portDiscoverer);
    communicator.writeLine("some data to write");

    QCOMPARE(spyPortDeleted.count(), 1);
    QCOMPARE(spyPortClosed.count(), 1);
    auto errorString = spyPortClosed.at(0).at(0).toString();
    QCOMPARE(errorString, "An error!!! ohoh");
    QCOMPARE(spyDataSent.count(), 0);
}

void MachineCommunicationTest::ifPortIsInErrorAfterReadClosePortAndEmitSignal()
{
    auto serialPort = new TestSerialPort();
    TestPortDiscovery portDiscoverer(serialPort);
    serialPort->setInError(true);

    MachineCommunication communicator;

    QSignalSpy spyPortDeleted(serialPort, &QObject::destroyed);
    QSignalSpy spyPortClosed(&communicator, &MachineCommunication::portClosed);
    QSignalSpy spyDataSent(&communicator, &MachineCommunication::dataSent);

    communicator.portFound(MachineInfo("a", "1"), &portDiscoverer);
    serialPort->simulateReceivedData("Toc toc...");

    QCOMPARE(spyPortDeleted.count(), 1);
    QCOMPARE(spyPortClosed.count(), 1);
    auto errorString = spyPortClosed.at(0).at(0).toString();
    QCOMPARE(errorString, "An error!!! ohoh");
    QCOMPARE(spyDataSent.count(), 0);
}

QTEST_GUILESS_MAIN(MachineCommunicationTest)

#include "machinecommunication_test.moc"

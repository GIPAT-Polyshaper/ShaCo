#include <memory>
#include <QtTest>
#include <QIODevice>
#include <QList>
#include <QSerialPort>
#include <QSignalSpy>
#include <QTime>
#include "core/machineinfo.h"
#include "core/portdiscovery.h"
#include "core/serialport.h"

class TestPortInfo {
public:
    TestPortInfo(quint16 idVendor = 0, quint16 idProduct = 0)
        : m_idVendor(idVendor)
        , m_idProduct(idProduct)
    {}

    quint16 productIdentifier() const
    {
        return m_idProduct;
    }

    quint16 vendorIdentifier() const
    {
        return m_idVendor;
    }

private:
    quint16 m_idVendor;
    quint16 m_idProduct;
};
Q_DECLARE_METATYPE(TestPortInfo)

// Not using the mock in testcommon, we need an ad-hoc implementation
class TestSerialPort : public SerialPortInterface {
    Q_OBJECT

public:
    TestSerialPort(bool errorOnOpen = false)
        : m_errorOnOpen(errorOnOpen)
    {
    }

    bool open() override
    {
        if (m_errorOnOpen) {
            emitErrorSignal();
        } else {
            emit portOpened();
        }

        return !m_errorOnOpen;
    }

    qint64 write(const QByteArray &data) override
    {
        emit dataWritten(data);

        return data.size();
    }

    QByteArray readAll() override
    {
        return m_readData;
    }

    QString errorString() const override // Not used in this test
    {
        throw QString("errorString should not be used in this test!!!");
    }

    void close() override // Not used in this test
    {
        throw QString("close should not be used in this test!!!");
    }

    void simulateReceivedData(QByteArray data)
    {
        m_readData = data;

        emit dataAvailable();
    }

    void emitErrorSignal()
    {
        emit errorOccurred();
    }

signals:
    void portOpened();
    void dataWritten(const QByteArray& data);

private:
    QByteArray m_readData;
    const bool m_errorOnOpen;
};

class PortDiscoveryTest : public QObject
{
    Q_OBJECT

public:
    PortDiscoveryTest();

signals:
    void portListingCalled();
    void serialPortCreated(TestPortInfo p);

private Q_SLOTS:
    void emitAMessageWhenStartProbing();
    void continuouslyProbeForPortsAtRegularIntervals();
    void openPortWhenTheExpectedVendorAndProductIdAreFound();
    void resetAndAskFirmwareVersionAfterOpeningPort();
    void askFirmwareVersionAgainIfNoAswerIsReceived();
    void ifTheExpectedReplyIsReceivedEmitSignal();
    void stopPollingWhenAValidPortIsFound();
    void continuePollingIfWrongAnswerIsReceived();
    void accumulateDataReceivedFromMachine();
    void askAgainForPortListAfterFailingTheMaximumNumberOfAttemptsOnAPort();
    void continueWithNextPortAfterFailingTheMaximumNumberOfAttemptsOnAPort();
    void discardAccumulatedDataWhenOpeningANewPort();
    void whenObtainPortIsCalledReturnPortAndDisconnectFromSignals();
    void deletePortAndContinueIfErrorSignalIsReceived();
    void ignorePortIfThereIsAnErrorWhenOpened();
    void doNotAskFirmwareVersionAgainIfPortFoundAfterAFailureAtOpening();
};

PortDiscoveryTest::PortDiscoveryTest()
{
    qRegisterMetaType<TestPortInfo>();
}

void PortDiscoveryTest::emitAMessageWhenStartProbing()
{
    auto portListingFunction = [](){ return QList<TestPortInfo>(); };
    auto serialPortFactory = [](TestPortInfo) { return std::unique_ptr<SerialPortInterface>(); };

    PortDiscovery<TestPortInfo> portDiscoverer(portListingFunction, serialPortFactory, 10000, 1, 1);

    QSignalSpy spy(&portDiscoverer, &AbstractPortDiscovery::startedDiscoveringPort);

    portDiscoverer.start();

    QCOMPARE(spy.count(), 1);
}

void PortDiscoveryTest::continuouslyProbeForPortsAtRegularIntervals()
{
    // Adding some delays to check that interval is between end of calls
    int count = 0;
    auto portListingFunction = [this, &count]() {
        if (count == 1) {
            QThread::msleep(200);
        }
        ++count;

        emit portListingCalled();
        return QList<TestPortInfo>();
    };
    auto serialPortFactory = [](TestPortInfo) { return std::unique_ptr<SerialPortInterface>(); };

    PortDiscovery<TestPortInfo> portDiscoverer(portListingFunction, serialPortFactory, 300, 1, 1);

    QSignalSpy spy(this, &PortDiscoveryTest::portListingCalled);

    QTime chrono;
    chrono.start();
    portDiscoverer.start();

    // Checking the port listing function is called multiple times (adding 100 ms of tolerance)
    QCOMPARE(spy.count(), 1); // first scan is immediate
    QVERIFY(spy.wait(650)); // second scan starts after 200 + 300 ms
    QVERIFY(chrono.restart() > 450);
    QVERIFY(spy.wait(450)); // third scan starts after 300 ms
    QVERIFY(chrono.elapsed() > 250);
}

void PortDiscoveryTest::openPortWhenTheExpectedVendorAndProductIdAreFound()
{
    auto portListingFunction = []() {
        return QList<TestPortInfo>{TestPortInfo(13, 17), TestPortInfo(0x2341, 0x0043), TestPortInfo()};
    };
    auto serialPort = new TestSerialPort();
    auto serialPortFactory = [this, serialPort](TestPortInfo p) {
        emit serialPortCreated(p);
        return std::unique_ptr<SerialPortInterface>(serialPort);
    };

    PortDiscovery<TestPortInfo> portDiscoverer(portListingFunction, serialPortFactory, 3000, 1, 1);

    QSignalSpy creationSpy(this, &PortDiscoveryTest::serialPortCreated);
    QSignalSpy openSpy(serialPort, &TestSerialPort::portOpened);

    portDiscoverer.start();

    QCOMPARE(creationSpy.count(), 1);
    auto portInfo = creationSpy.at(0).at(0).value<TestPortInfo>();
    QCOMPARE(portInfo.vendorIdentifier(), 0x2341);
    QCOMPARE(portInfo.productIdentifier(), 0x0043);
    QCOMPARE(openSpy.count(), 1);
}

void PortDiscoveryTest::resetAndAskFirmwareVersionAfterOpeningPort()
{
    TestPortInfo portInfo(0x2341, 0x0043);
    auto serialPort = new TestSerialPort();
    auto portListingFunction = [&portInfo]() { return QList<TestPortInfo>{portInfo}; };
    auto serialPortFactory = [serialPort](TestPortInfo) { return std::unique_ptr<SerialPortInterface>(serialPort); };

    PortDiscovery<TestPortInfo> portDiscoverer(portListingFunction, serialPortFactory, 3000, 1000, 5);

    QSignalSpy spy(serialPort, &TestSerialPort::dataWritten);

    portDiscoverer.start();

    QCOMPARE(spy.count(), 2);
    QCOMPARE(spy.at(0).at(0).toByteArray(), "\xC0");
    QCOMPARE(spy.at(1).at(0).toByteArray(), "$I\n");
}

void PortDiscoveryTest::askFirmwareVersionAgainIfNoAswerIsReceived()
{
    TestPortInfo portInfo(0x2341, 0x0043);
    auto serialPort = new TestSerialPort();
    auto portListingFunction = [&portInfo]() { return QList<TestPortInfo>{portInfo}; };
    auto serialPortFactory = [serialPort](TestPortInfo) { return std::unique_ptr<SerialPortInterface>(serialPort); };

    PortDiscovery<TestPortInfo> portDiscoverer(portListingFunction, serialPortFactory, 3000, 300, 5);

    QSignalSpy spy(serialPort, &TestSerialPort::dataWritten);

    QTime chrono;
    chrono.start();
    portDiscoverer.start();

    // First is immediate
    QCOMPARE(spy.count(), 2);
    QCOMPARE(spy.at(0).at(0).toByteArray(), "\xC0");
    QCOMPARE(spy.at(1).at(0).toByteArray(), "$I\n");

    QVERIFY(spy.wait(450));
    QVERIFY(chrono.restart() > 250);
    QCOMPARE(spy.at(2).at(0).toByteArray(), "$I\n");

    QVERIFY(spy.wait(450));
    QVERIFY(chrono.elapsed() > 250);
    QCOMPARE(spy.at(3).at(0).toByteArray(), "$I\n");
}

void PortDiscoveryTest::ifTheExpectedReplyIsReceivedEmitSignal()
{
    TestPortInfo portInfo(0x2341, 0x0043);
    auto serialPort = new TestSerialPort();
    auto portListingFunction = [&portInfo]() { return QList<TestPortInfo>{portInfo}; };
    auto serialPortFactory = [serialPort](TestPortInfo) { return std::unique_ptr<SerialPortInterface>(serialPort); };

    PortDiscovery<TestPortInfo> portDiscoverer(portListingFunction, serialPortFactory, 300, 300, 5);

    QSignalSpy spy(&portDiscoverer, &PortDiscovery<TestPortInfo>::portFound);

    portDiscoverer.start();

    serialPort->simulateReceivedData("[PolyShaper Oranje][pn123 sn456 789]ok\r\n");

    QCOMPARE(spy.count(), 1);
    auto machineInfo = spy.at(0).at(0).value<MachineInfo>();
    QCOMPARE(machineInfo.machineName(), "Oranje");
    QCOMPARE(machineInfo.partNumber(), "pn123");
    QCOMPARE(machineInfo.serialNumber(), "sn456");
    QCOMPARE(machineInfo.firmwareVersion(), "789");
    QCOMPARE(spy.at(0).at(1).value<AbstractPortDiscovery*>(), &portDiscoverer);
}

void PortDiscoveryTest::stopPollingWhenAValidPortIsFound()
{
    TestPortInfo portInfo(0x2341, 0x0043);
    auto serialPort = new TestSerialPort();
    auto portListingFunction = [&portInfo]() { return QList<TestPortInfo>{portInfo}; };
    auto serialPortFactory = [serialPort](TestPortInfo) { return std::unique_ptr<SerialPortInterface>(serialPort); };

    PortDiscovery<TestPortInfo> portDiscoverer(portListingFunction, serialPortFactory, 300, 300, 5);

    QSignalSpy spy(serialPort, &TestSerialPort::dataWritten);

    portDiscoverer.start();

    serialPort->simulateReceivedData("[PolyShaper Oranje][pn123 sn456 789]ok\r\n");

    QCOMPARE(spy.count(), 2); // hard reset + request of information

    // There should be no more requests
    QVERIFY(!spy.wait(600));
}

void PortDiscoveryTest::continuePollingIfWrongAnswerIsReceived()
{
    TestPortInfo portInfo(0x2341, 0x0043);
    auto serialPort = new TestSerialPort();
    auto portListingFunction = [&portInfo]() { return QList<TestPortInfo>{portInfo}; };
    auto serialPortFactory = [serialPort](TestPortInfo) { return std::unique_ptr<SerialPortInterface>(serialPort); };

    PortDiscovery<TestPortInfo> portDiscoverer(portListingFunction, serialPortFactory, 300, 300, 5);

    QSignalSpy spy(serialPort, &TestSerialPort::dataWritten);

    portDiscoverer.start();

    serialPort->simulateReceivedData("invalid something\r\n");

    QCOMPARE(spy.count(), 2); // hard reset + request for information

    // There should be more requests
    QVERIFY(spy.wait(450));
}

void PortDiscoveryTest::accumulateDataReceivedFromMachine()
{
    TestPortInfo portInfo(0x2341, 0x0043);
    auto serialPort = new TestSerialPort();
    auto portListingFunction = [&portInfo]() { return QList<TestPortInfo>{portInfo}; };
    auto serialPortFactory = [serialPort](TestPortInfo) { return std::unique_ptr<SerialPortInterface>(serialPort); };

    PortDiscovery<TestPortInfo> portDiscoverer(portListingFunction, serialPortFactory, 300, 300, 5);

    QSignalSpy spy(&portDiscoverer, &PortDiscovery<TestPortInfo>::portFound);

    portDiscoverer.start();

    serialPort->simulateReceivedData("[PolyShap");
    serialPort->simulateReceivedData("er Oranje][pn123 sn45");
    serialPort->simulateReceivedData("6 789]ok\r\n");

    QCOMPARE(spy.count(), 1);
    auto machineInfo = spy.at(0).at(0).value<MachineInfo>();
    QCOMPARE(machineInfo.machineName(), "Oranje");
    QCOMPARE(machineInfo.partNumber(), "pn123");
    QCOMPARE(machineInfo.serialNumber(), "sn456");
    QCOMPARE(machineInfo.firmwareVersion(), "789");
    QCOMPARE(spy.at(0).at(1).value<AbstractPortDiscovery*>(), &portDiscoverer);
}

void PortDiscoveryTest::askAgainForPortListAfterFailingTheMaximumNumberOfAttemptsOnAPort()
{
    TestPortInfo portInfo(0x2341, 0x0043);
    auto portListingFunction = [this, &portInfo]() {
        emit portListingCalled();
        return QList<TestPortInfo>{portInfo};
    };
    auto serialPort1 = new TestSerialPort();
    auto serialPort2 = new TestSerialPort();
    bool first = true;
    auto serialPortFactory = [serialPort1, serialPort2, &first](TestPortInfo) {
        if (first) {
            first = false;
            return std::unique_ptr<SerialPortInterface>(serialPort1);
        } else {
            return std::unique_ptr<SerialPortInterface>(serialPort2);
        }
    };

    PortDiscovery<TestPortInfo> portDiscoverer(portListingFunction, serialPortFactory, 500, 100, 3);

    QSignalSpy portListingSpy(this, &PortDiscoveryTest::portListingCalled);
    QSignalSpy dataWrittenSpy1(serialPort1, &TestSerialPort::dataWritten);
    QSignalSpy dataWrittenSpy2(serialPort2, &TestSerialPort::dataWritten);

    portDiscoverer.start();

    QCOMPARE(portListingSpy.count(), 1);

    // First is immediate
    QCOMPARE(dataWrittenSpy1.count(), 2);
    QCOMPARE(dataWrittenSpy1.at(0).at(0).toByteArray(), "\xC0");
    QCOMPARE(dataWrittenSpy1.at(1).at(0).toByteArray(), "$I\n");

    QVERIFY(dataWrittenSpy1.wait(250));
    QCOMPARE(dataWrittenSpy1.at(2).at(0).toByteArray(), "$I\n");

    QVERIFY(dataWrittenSpy1.wait(250));
    QCOMPARE(dataWrittenSpy1.at(3).at(0).toByteArray(), "$I\n");

    // Now it should call listing again after the delay and restart polling port
    QVERIFY(portListingSpy.wait(650));
    QCOMPARE(dataWrittenSpy2.count(), 2);
    QCOMPARE(dataWrittenSpy2.at(0).at(0).toByteArray(), "\xC0");
    QCOMPARE(dataWrittenSpy2.at(1).at(0).toByteArray(), "$I\n");

    QVERIFY(dataWrittenSpy2.wait(250));
    QCOMPARE(dataWrittenSpy2.at(2).at(0).toByteArray(), "$I\n");
}

void PortDiscoveryTest::continueWithNextPortAfterFailingTheMaximumNumberOfAttemptsOnAPort()
{
    TestPortInfo portInfo(0x2341, 0x0043);
    auto portListingFunction = [this, &portInfo]() {
        emit portListingCalled();
        return QList<TestPortInfo>{portInfo, TestPortInfo(1, 2), portInfo};
    };
    auto serialPort1 = new TestSerialPort();
    auto serialPort2 = new TestSerialPort();
    bool first = true;
    auto serialPortFactory = [serialPort1, serialPort2, &first](TestPortInfo) {
        if (first) {
            first = false;
            return std::unique_ptr<SerialPortInterface>(serialPort1);
        } else {
            return std::unique_ptr<SerialPortInterface>(serialPort2);
        }
    };

    PortDiscovery<TestPortInfo> portDiscoverer(portListingFunction, serialPortFactory, 10, 100, 3);

    QSignalSpy portListingSpy(this, &PortDiscoveryTest::portListingCalled);
    QSignalSpy dataWrittenSpy1(serialPort1, &TestSerialPort::dataWritten);
    QSignalSpy dataWrittenSpy2(serialPort2, &TestSerialPort::dataWritten);

    portDiscoverer.start();

    QCOMPARE(portListingSpy.count(), 1);

    // First is immediate
    QCOMPARE(dataWrittenSpy1.count(), 2);
    QCOMPARE(dataWrittenSpy1.at(0).at(0).toByteArray(), "\xC0");
    QCOMPARE(dataWrittenSpy1.at(1).at(0).toByteArray(), "$I\n");

    QVERIFY(dataWrittenSpy1.wait(250));
    QCOMPARE(dataWrittenSpy1.at(2).at(0).toByteArray(), "$I\n");

    QVERIFY(dataWrittenSpy1.wait(250));
    QCOMPARE(dataWrittenSpy1.at(3).at(0).toByteArray(), "$I\n");

    // Now it should move to the following port immediately, then continue via polling
    QVERIFY(dataWrittenSpy2.wait(250));
    QCOMPARE(dataWrittenSpy2.count(), 2);
    QCOMPARE(dataWrittenSpy2.at(0).at(0).toByteArray(), "\xC0");
    QCOMPARE(dataWrittenSpy2.at(1).at(0).toByteArray(), "$I\n");
    QCOMPARE(portListingSpy.count(), 1);

    QVERIFY(dataWrittenSpy2.wait(250));
    QCOMPARE(dataWrittenSpy2.at(2).at(0).toByteArray(), "$I\n");
}

void PortDiscoveryTest::discardAccumulatedDataWhenOpeningANewPort()
{
    // To test data is discarded we send part of a correct reply to the first port and part to the
    // second one, but portFound hsould not be emitted

    TestPortInfo portInfo(0x2341, 0x0043);
    auto portListingFunction = [this, &portInfo]() {
        emit portListingCalled();
        return QList<TestPortInfo>{portInfo, TestPortInfo(1, 2), portInfo};
    };
    auto serialPort1 = new TestSerialPort();
    auto serialPort2 = new TestSerialPort();
    bool first = true;
    auto serialPortFactory = [serialPort1, serialPort2, &first](TestPortInfo) {
        if (first) {
            first = false;
            return std::unique_ptr<SerialPortInterface>(serialPort1);
        } else {
            return std::unique_ptr<SerialPortInterface>(serialPort2);
        }
    };

    PortDiscovery<TestPortInfo> portDiscoverer(portListingFunction, serialPortFactory, 10, 100, 3);

    QSignalSpy portFoundSpy(&portDiscoverer, &PortDiscovery<TestPortInfo>::portFound);
    QSignalSpy dataWrittenSpy1(serialPort1, &TestSerialPort::dataWritten);
    QSignalSpy dataWrittenSpy2(serialPort2, &TestSerialPort::dataWritten);

    portDiscoverer.start();

    // First is immediate
    QCOMPARE(dataWrittenSpy1.count(), 2);
    QCOMPARE(dataWrittenSpy1.at(0).at(0).toByteArray(), "\xC0");
    QCOMPARE(dataWrittenSpy1.at(1).at(0).toByteArray(), "$I\n");

    serialPort1->simulateReceivedData("[PolyShaper Oran");

    QVERIFY(dataWrittenSpy1.wait(250));
    QCOMPARE(dataWrittenSpy1.at(2).at(0).toByteArray(), "$I\n");

    QVERIFY(dataWrittenSpy1.wait(250));
    QCOMPARE(dataWrittenSpy1.at(3).at(0).toByteArray(), "$I\n");

    // Now it should move to the following port immediately, then continue via polling
    QVERIFY(dataWrittenSpy2.wait(250));
    QCOMPARE(dataWrittenSpy2.count(), 2);
    QCOMPARE(dataWrittenSpy2.at(0).at(0).toByteArray(), "\xC0");
    QCOMPARE(dataWrittenSpy2.at(1).at(0).toByteArray(), "$I\n");

    serialPort2->simulateReceivedData("je][1.2]ok\r\n");

    // Should not stop and continue polling
    QVERIFY(dataWrittenSpy2.wait(250));
    QCOMPARE(dataWrittenSpy2.at(2).at(0).toByteArray(), "$I\n");
    QCOMPARE(portFoundSpy.count(), 0);
}

void PortDiscoveryTest::whenObtainPortIsCalledReturnPortAndDisconnectFromSignals()
{
    TestPortInfo portInfo(0x2341, 0x0043);
    auto serialPort = new TestSerialPort();
    auto portListingFunction = [&portInfo]() { return QList<TestPortInfo>{portInfo}; };
    auto serialPortFactory = [serialPort](TestPortInfo) { return std::unique_ptr<SerialPortInterface>(serialPort); };

    PortDiscovery<TestPortInfo> portDiscoverer(portListingFunction, serialPortFactory, 300, 300, 5);

    QSignalSpy spy(&portDiscoverer, &PortDiscovery<TestPortInfo>::portFound);

    portDiscoverer.start();

    serialPort->simulateReceivedData("[PolyShaper Oranje][pn123 sn456 789]ok\r\n");

    QCOMPARE(spy.count(), 1);
    auto foundPort = portDiscoverer.obtainPort();
    QCOMPARE(serialPort, foundPort.get());

    // Sending data again, nothing should happen
    serialPort->simulateReceivedData("[PolyShaper Oranje][pn123 sn456 789]ok\r\n");
    QCOMPARE(spy.count(), 1);
}

void PortDiscoveryTest::deletePortAndContinueIfErrorSignalIsReceived()
{
    TestPortInfo portInfo(0x2341, 0x0043);
    auto portListingFunction = [&portInfo]() {
        return QList<TestPortInfo>{portInfo, TestPortInfo(1, 2), portInfo};
    };
    auto serialPort1 = new TestSerialPort();
    auto serialPort2 = new TestSerialPort();
    bool first = true;
    auto serialPortFactory = [serialPort1, serialPort2, &first](TestPortInfo) {
        if (first) {
            first = false;
            return std::unique_ptr<SerialPortInterface>(serialPort1);
        } else {
            return std::unique_ptr<SerialPortInterface>(serialPort2);
        }
    };

    PortDiscovery<TestPortInfo> portDiscoverer(portListingFunction, serialPortFactory, 10, 100, 3);

    QSignalSpy dataWrittenSpy1(serialPort1, &TestSerialPort::dataWritten);
    QSignalSpy portDeletedSpy(serialPort1, &TestSerialPort::destroyed);
    QSignalSpy dataWrittenSpy2(serialPort2, &TestSerialPort::dataWritten);

    portDiscoverer.start();

    // First is immediate
    QCOMPARE(dataWrittenSpy1.count(), 2);
    QCOMPARE(dataWrittenSpy1.at(0).at(0).toByteArray(), "\xC0");
    QCOMPARE(dataWrittenSpy1.at(1).at(0).toByteArray(), "$I\n");

    // Port is in error
    serialPort1->emitErrorSignal();
    QCOMPARE(portDeletedSpy.count(), 1);

    // Now it should move to the following port immediately, then continue via polling
    QCOMPARE(dataWrittenSpy2.count(), 2);
    QCOMPARE(dataWrittenSpy2.at(0).at(0).toByteArray(), "\xC0");
    QCOMPARE(dataWrittenSpy2.at(1).at(0).toByteArray(), "$I\n");

    QVERIFY(dataWrittenSpy2.wait(250));
    QCOMPARE(dataWrittenSpy2.at(2).at(0).toByteArray(), "$I\n");
}

void PortDiscoveryTest::ignorePortIfThereIsAnErrorWhenOpened()
{
    TestPortInfo portInfo(0x2341, 0x0043);
    auto portListingFunction = [&portInfo]() { return QList<TestPortInfo>{portInfo}; };
    auto serialPort1 = new TestSerialPort(true);
    auto serialPort2 = new TestSerialPort(true);
    auto first = true;
    auto serialPortFactory = [serialPort1, serialPort2, &first](TestPortInfo) {
        if (first) {
            first = false;
            return std::unique_ptr<SerialPortInterface>(serialPort1);
        } else {
            return std::unique_ptr<SerialPortInterface>(serialPort2);
        }
    };

    PortDiscovery<TestPortInfo> portDiscoverer(portListingFunction, serialPortFactory, 10, 100, 3);

    // This should simply not crash
    portDiscoverer.start();

    // Another round
    QThread::msleep(200);
    QCoreApplication::processEvents();
}

void PortDiscoveryTest::doNotAskFirmwareVersionAgainIfPortFoundAfterAFailureAtOpening()
{
    // This basically tests that there are no nested calls to searchPort()
    TestPortInfo portInfo(0x2341, 0x0043);
    auto portListingFunction = [&portInfo]() {
        return QList<TestPortInfo>{portInfo, portInfo};
    };
    auto serialPort1 = new TestSerialPort(true);
    auto serialPort2 = new TestSerialPort();
    bool first = true;
    auto serialPortFactory = [serialPort1, serialPort2, &first](TestPortInfo) {
        if (first) {
            first = false;
            return std::unique_ptr<SerialPortInterface>(serialPort1);
        } else {
            return std::unique_ptr<SerialPortInterface>(serialPort2);
        }
    };

    PortDiscovery<TestPortInfo> portDiscoverer(portListingFunction, serialPortFactory, 10, 100, 3);

    QSignalSpy dataWrittenSpy2(serialPort2, &TestSerialPort::dataWritten);

    portDiscoverer.start();

    // Now it should move to the following port immediately, then continue via polling. The bug was
    // that we received a double reset and request
    QCOMPARE(dataWrittenSpy2.count(), 2);
    QCOMPARE(dataWrittenSpy2.at(0).at(0).toByteArray(), "\xC0");
    QCOMPARE(dataWrittenSpy2.at(1).at(0).toByteArray(), "$I\n");

    // Polling again
    QVERIFY(dataWrittenSpy2.wait(250));
    QCOMPARE(dataWrittenSpy2.at(2).at(0).toByteArray(), "$I\n");
}

QTEST_GUILESS_MAIN(PortDiscoveryTest)
#include "portdiscovery_test.moc"

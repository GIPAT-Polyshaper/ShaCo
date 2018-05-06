#include <memory>
#include <utility>
#include <QtTest>
#include <QFlags>
#include <QIODevice>
#include <QList>
#include <QSerialPort>
#include <QSignalSpy>
#include <QThread>
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

class TestSerialPort : public SerialPortInterface {
    Q_OBJECT

public:
    TestSerialPort()
        : SerialPortInterface()
        , m_answers()
    {}

    bool open(QIODevice::OpenMode mode, qint32 baudRate) override
    {
        emit portOpened(mode == QIODevice::ReadWrite, baudRate);

        return true;
    }

    qint64 write(const QByteArray &data) override
    {
        emit dataWritten(data);

        return data.size();
    }

    QByteArray read(int) override
    {
        if (m_answers.isEmpty()) {
            return QByteArray();
        }

        auto answer = m_answers.takeFirst();

        QThread::msleep(answer.second);
        return answer.first;
    }

    // A list of couples: answers and time after which it has to be returned (in milliseconds)
    void setAnswers(QList<std::pair<QByteArray, int>> answers)
    {
        m_answers = answers;
    }

signals:
    void portOpened(bool modeIsReadWrite, qint32 baudRate);
    void dataWritten(const QByteArray& data);

private:
    QList<std::pair<QByteArray, int>> m_answers;
};

// TODO-TOMMY: Modificare leggermente grbl in modo da fargli tornare la versione della macchina per essere sicuri che nel discovery prendiamo una nostra macchina - Vedi comando $I
class PortDiscoveryTest : public QObject
{
    Q_OBJECT

public:
    PortDiscoveryTest();

signals:
    void portListingFactoryCalled();
    void serialPortCreated(TestPortInfo p);

private Q_SLOTS:
    void emitAMessageWhenStartProbing();
    void continuouslyProbeForPortsAtRegularIntervals();
    void openPortWhenTheExpectedVendorAndProductIdAreFound();
    void askFirmwareVersionAfterOpeningPort();
    void signalPortFoundIfFirmwareReturnsTheCorrectVersion();
    void stopScanningAfterPortFound();
    void ignorePortIfAnswerIsNotTheExpectedOne();
    void onlyOpenTheFirstFoundPortInList();
    void keepReadingUntilOkIsReceived();
};

PortDiscoveryTest::PortDiscoveryTest()
{
    qRegisterMetaType<TestPortInfo>();
}

void PortDiscoveryTest::emitAMessageWhenStartProbing()
{
    auto portListingFunction = [](){ return QList<TestPortInfo>(); };
    auto serialPortFactory = [](TestPortInfo) { return std::unique_ptr<SerialPortInterface>(); };

    PortDiscovery<TestPortInfo> portDiscovery(portListingFunction, serialPortFactory, 10000, 100);

    QSignalSpy spy(&portDiscovery, &PortDiscoverySignalSlots::startedDiscoveringPort);

    portDiscovery.start();

    QCOMPARE(spy.count(), 1);
}

void PortDiscoveryTest::continuouslyProbeForPortsAtRegularIntervals()
{
    auto portListingFunction = [this]() { emit portListingFactoryCalled(); return QList<TestPortInfo>(); };
    auto serialPortFactory = [](TestPortInfo) { return std::unique_ptr<SerialPortInterface>(); };

    PortDiscovery<TestPortInfo> portDiscovery(portListingFunction, serialPortFactory, 300, 100);

    QSignalSpy spy(this, &PortDiscoveryTest::portListingFactoryCalled);

    portDiscovery.start();

    // Checking the port listing function is called multiple times
    QVERIFY(spy.wait(600));
    QVERIFY(spy.wait(600));
    QVERIFY(spy.wait(600));
}

void PortDiscoveryTest::openPortWhenTheExpectedVendorAndProductIdAreFound()
{
    auto portListingFunction = []() { return QList<TestPortInfo>{TestPortInfo(13, 17), TestPortInfo(0x2341, 0x0043), TestPortInfo()}; };
    auto serialPortFactory = [this](TestPortInfo p) { emit serialPortCreated(p); return std::make_unique<TestSerialPort>(); };

    PortDiscovery<TestPortInfo> portDiscovery(portListingFunction, serialPortFactory, 3000, 100);

    QSignalSpy spy(this, &PortDiscoveryTest::serialPortCreated);

    portDiscovery.start();

    QCOMPARE(spy.count(), 1);
    auto portInfo = spy.at(0).at(0).value<TestPortInfo>();
    QCOMPARE(portInfo.vendorIdentifier(), 0x2341);
    QCOMPARE(portInfo.productIdentifier(), 0x0043);
}

void PortDiscoveryTest::askFirmwareVersionAfterOpeningPort()
{
    TestPortInfo portInfo(0x2341, 0x0043);
    auto serialPort = new TestSerialPort();
    auto portListingFunction = [&portInfo]() { return QList<TestPortInfo>{portInfo}; };
    auto serialPortFactory = [serialPort](TestPortInfo) { return std::unique_ptr<SerialPortInterface>(serialPort); };

    PortDiscovery<TestPortInfo> portDiscovery(portListingFunction, serialPortFactory, 3000, 100);

    QSignalSpy spyOpen(serialPort, &TestSerialPort::portOpened);
    QSignalSpy spyWrite(serialPort, &TestSerialPort::dataWritten);

    portDiscovery.start();

    QCOMPARE(spyOpen.count(), 1);
    auto modeIsReadWrite = spyOpen.at(0).at(0).value<bool>();
    auto baudRate = spyOpen.at(0).at(1).value<qint32>();
    QVERIFY(modeIsReadWrite);
    QCOMPARE(baudRate, QSerialPort::Baud115200);
    QCOMPARE(spyWrite.count(), 1);
    auto writtenData = spyWrite.at(0).at(0).toByteArray();
    QCOMPARE(writtenData, "$I\n");
}

void PortDiscoveryTest::signalPortFoundIfFirmwareReturnsTheCorrectVersion()
{
    TestPortInfo portInfo(0x2341, 0x0043);
    auto serialPort = new TestSerialPort();
    serialPort->setAnswers({{"[PolyShaper Oranje][1.2]ok\r\n", 0}});
    auto portListingFunction = [&portInfo]() { return QList<TestPortInfo>{portInfo}; };
    auto serialPortFactory = [serialPort](TestPortInfo) { return std::unique_ptr<SerialPortInterface>(serialPort); };

    PortDiscovery<TestPortInfo> portDiscovery(portListingFunction, serialPortFactory, 3000, 100);

    QSignalSpy spy(&portDiscovery, &PortDiscovery<TestPortInfo>::portFound);

    portDiscovery.start();

    QCOMPARE(spy.count(), 1);
    auto foundPort = portDiscovery.obtainPort();
    auto machineInfo = spy.at(0).at(0).value<MachineInfo>();
    QCOMPARE(serialPort, foundPort.get());
    QCOMPARE(machineInfo.machineName(), "Oranje");
    QCOMPARE(machineInfo.firmwareVersion(), "1.2");
}

void PortDiscoveryTest::stopScanningAfterPortFound()
{
    TestPortInfo portInfo(0x2341, 0x0043);
    auto portListingFunction = [&portInfo, this]() { emit portListingFactoryCalled(); return QList<TestPortInfo>{portInfo}; };
    auto serialPortFactory = [](TestPortInfo) {
        auto port = std::make_unique<TestSerialPort>();
        port->setAnswers({{"[PolyShaper Oranje][1.2]ok\r\n", 0}});
        return port;
    };

    PortDiscovery<TestPortInfo> portDiscovery(portListingFunction, serialPortFactory, 300, 100);

    QSignalSpy spyFound(&portDiscovery, &PortDiscovery<TestPortInfo>::portFound);
    QSignalSpy spyPortListing(this, &PortDiscoveryTest::portListingFactoryCalled);

    portDiscovery.start();

    QCOMPARE(spyFound.count(), 1);
    QVERIFY(!spyPortListing.wait(600));
}

void PortDiscoveryTest::ignorePortIfAnswerIsNotTheExpectedOne()
{
    TestPortInfo portInfo(0x2341, 0x0043);
    auto portListingFunction = [&portInfo, this]() { emit portListingFactoryCalled(); return QList<TestPortInfo>{TestPortInfo(), portInfo}; };
    auto serialPortFactory = [](TestPortInfo) { auto port = std::make_unique<TestSerialPort>(); port->setAnswers({{"wrong ok\r\n", 0}}); return port; };

    PortDiscovery<TestPortInfo> portDiscovery(portListingFunction, serialPortFactory, 300, 100);

    QSignalSpy spyFound(&portDiscovery, &PortDiscovery<TestPortInfo>::portFound);
    QSignalSpy spyPortListing(this, &PortDiscoveryTest::portListingFactoryCalled);

    portDiscovery.start();

    // No port found, scanning continues
    QVERIFY(spyPortListing.wait(600));
    QVERIFY(spyPortListing.wait(600));
    QCOMPARE(spyFound.count(), 0);
}

void PortDiscoveryTest::onlyOpenTheFirstFoundPortInList()
{
    TestPortInfo portInfo(0x2341, 0x0043);
    auto portListingFunction = [&portInfo, this]() { return QList<TestPortInfo>{portInfo, portInfo}; };
    auto serialPortFactory = [this](TestPortInfo p) {
        emit serialPortCreated(p);
        auto port = std::make_unique<TestSerialPort>();
        port->setAnswers({{"[PolyShaper Oranje][1.2]ok\r\n", 0}});
        return port;
    };

    PortDiscovery<TestPortInfo> portDiscovery(portListingFunction, serialPortFactory, 300, 100);

    QSignalSpy spyPortCreated(this, &PortDiscoveryTest::serialPortCreated);
    QSignalSpy spyFound(&portDiscovery, &PortDiscovery<TestPortInfo>::portFound);

    portDiscovery.start();

    QCOMPARE(spyFound.count(), 1);
    QCOMPARE(spyPortCreated.count(), 1);
}

void PortDiscoveryTest::keepReadingUntilOkIsReceived()
{
    TestPortInfo portInfo(0x2341, 0x0043);
    auto portListingFunction = [&portInfo, this]() { return QList<TestPortInfo>{portInfo}; };
    auto serialPortFactory = [this](TestPortInfo p) {
        emit serialPortCreated(p);
        auto port = std::make_unique<TestSerialPort>();
        port->setAnswers({{"[PolyShap", 0}, {"er Oranje][1", 0}, {".2]ok\r\n", 0}});
        return port;
    };

    PortDiscovery<TestPortInfo> portDiscovery(portListingFunction, serialPortFactory, 3000, 100);

    QSignalSpy spy(&portDiscovery, &PortDiscovery<TestPortInfo>::portFound);

    portDiscovery.start();

    QCOMPARE(spy.count(), 1);
}

QTEST_GUILESS_MAIN(PortDiscoveryTest)
#include "portdiscovery_test.moc"

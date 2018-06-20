#include <memory>
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QSignalSpy>
#include <QString>
#include <QtTest>
#include "core/machinecommunication.h"
#include "core/portdiscovery.h"
#include "core/serialport.h"
#include "testcommon/testportdiscovery.h"
#include "testcommon/testserialport.h"

class MachineCommunicationTest : public QObject
{
    Q_OBJECT

public:
    MachineCommunicationTest();

private Q_SLOTS:
    void grabPortFromPortDiscovererWhenMachineFoundIsCalled();
    void writeDataToSerialPort();
    void writeLineToSerialPort();
    void doNotWriteLineIfNoSerialPortIsPresent();
    void emitSignalWhenDataIsWritten();
    void emitSignalWhenDataIsReceived();
    void ifPortIsInErrorAfterWriteClosePortAndEmitSignal();
    void ifPortIsInErrorAfterReadClosePortAndEmitSignal();
    void closePortWithError();
    void closePortWithoutError();
    void emitSignalWhenPortIsGrabbedFromPortDiscovery();
    void sendFeedHoldWhenAskedTo();
    void sendResumeFeedHoldWhenAskedTo();
    void sendSoftResetWhenAskedTo();
    void doHardResetWhenAskedTo();
};

MachineCommunicationTest::MachineCommunicationTest()
{
}

void MachineCommunicationTest::grabPortFromPortDiscovererWhenMachineFoundIsCalled()
{
    auto serialPort = new TestSerialPort();
    TestPortDiscovery portDiscoverer(serialPort);

    MachineCommunication communicator(100);

    QSignalSpy spy(&portDiscoverer, &TestPortDiscovery::serialPortMoved);

    communicator.portFound(MachineInfo("a", "1"), &portDiscoverer);

    QCOMPARE(spy.count(), 1);
}

void MachineCommunicationTest::doNotWriteLineIfNoSerialPortIsPresent()
{
    MachineCommunication communicator(100);

    // This should simply not crash
    communicator.writeLine("some data to write");
}

void MachineCommunicationTest::writeDataToSerialPort()
{
    auto serialPort = new TestSerialPort();
    TestPortDiscovery portDiscoverer(serialPort);

    MachineCommunication communicator(100);

    communicator.portFound(MachineInfo("a", "1"), &portDiscoverer);
    communicator.writeData("some data to write");

    QCOMPARE(serialPort->writtenData(), "some data to write");
}

void MachineCommunicationTest::writeLineToSerialPort()
{
    auto serialPort = new TestSerialPort();
    TestPortDiscovery portDiscoverer(serialPort);

    MachineCommunication communicator(100);

    communicator.portFound(MachineInfo("a", "1"), &portDiscoverer);
    communicator.writeLine("some data to write");

    QCOMPARE(serialPort->writtenData(), "some data to write\n");
}

void MachineCommunicationTest::emitSignalWhenDataIsWritten()
{
    auto serialPort = new TestSerialPort();
    TestPortDiscovery portDiscoverer(serialPort);

    MachineCommunication communicator(100);

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

    MachineCommunication communicator(100);

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

    MachineCommunication communicator(100);

    QSignalSpy spyPortDeleted(serialPort, &QObject::destroyed);
    QSignalSpy spyPortClosed(&communicator, &MachineCommunication::portClosedWithError);
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

    MachineCommunication communicator(100);

    QSignalSpy spyPortDeleted(serialPort, &QObject::destroyed);
    QSignalSpy spyPortClosed(&communicator, &MachineCommunication::portClosedWithError);
    QSignalSpy spyDataSent(&communicator, &MachineCommunication::dataSent);

    communicator.portFound(MachineInfo("a", "1"), &portDiscoverer);
    serialPort->simulateReceivedData("Toc toc...");

    QCOMPARE(spyPortDeleted.count(), 1);
    QCOMPARE(spyPortClosed.count(), 1);
    auto errorString = spyPortClosed.at(0).at(0).toString();
    QCOMPARE(errorString, "An error!!! ohoh");
    QCOMPARE(spyDataSent.count(), 0);
}

void MachineCommunicationTest::closePortWithError()
{
    auto serialPort = new TestSerialPort();
    TestPortDiscovery portDiscoverer(serialPort);

    MachineCommunication communicator(100);

    QSignalSpy spyPortDeleted(serialPort, &QObject::destroyed);
    QSignalSpy spyPortClosed(&communicator, &MachineCommunication::portClosedWithError);
    QSignalSpy spyDataSent(&communicator, &MachineCommunication::dataSent);

    communicator.portFound(MachineInfo("a", "1"), &portDiscoverer);
    communicator.closePortWithError("a generated error");

    QCOMPARE(spyPortDeleted.count(), 1);
    QCOMPARE(spyPortClosed.count(), 1);
    auto errorString = spyPortClosed.at(0).at(0).toString();
    QCOMPARE(errorString, "a generated error");
    QCOMPARE(spyDataSent.count(), 0);
}

void MachineCommunicationTest::closePortWithoutError()
{
    auto serialPort = new TestSerialPort();
    TestPortDiscovery portDiscoverer(serialPort);

    MachineCommunication communicator(100);

    QSignalSpy spyPortDeleted(serialPort, &QObject::destroyed);
    QSignalSpy spyPortClosed(&communicator, &MachineCommunication::portClosed);
    QSignalSpy spyDataSent(&communicator, &MachineCommunication::dataSent);

    communicator.portFound(MachineInfo("a", "1"), &portDiscoverer);
    communicator.closePort();

    QCOMPARE(spyPortDeleted.count(), 1);
    QCOMPARE(spyPortClosed.count(), 1);
    QCOMPARE(spyDataSent.count(), 0);
}

void MachineCommunicationTest::emitSignalWhenPortIsGrabbedFromPortDiscovery()
{
    auto serialPort = new TestSerialPort();
    TestPortDiscovery portDiscoverer(serialPort);

    MachineCommunication communicator(100);

    QSignalSpy spy(&communicator, &MachineCommunication::machineInitialized);

    communicator.portFound(MachineInfo("a", "1"), &portDiscoverer);

    QCOMPARE(spy.count(), 1);
}

void MachineCommunicationTest::sendFeedHoldWhenAskedTo()
{
    auto serialPort = new TestSerialPort();
    TestPortDiscovery portDiscoverer(serialPort);

    MachineCommunication communicator(100);

    QSignalSpy spy(&communicator, &MachineCommunication::dataSent);

    communicator.portFound(MachineInfo("a", "1"), &portDiscoverer);
    communicator.feedHold();

    QCOMPARE(spy.count(), 1);
    auto data = spy.at(0).at(0).toByteArray();
    QCOMPARE(data, "!");
}

void MachineCommunicationTest::sendResumeFeedHoldWhenAskedTo()
{
    auto serialPort = new TestSerialPort();
    TestPortDiscovery portDiscoverer(serialPort);

    MachineCommunication communicator(100);

    QSignalSpy spy(&communicator, &MachineCommunication::dataSent);

    communicator.portFound(MachineInfo("a", "1"), &portDiscoverer);
    communicator.resumeFeedHold();

    QCOMPARE(spy.count(), 1);
    auto data = spy.at(0).at(0).toByteArray();
    QCOMPARE(data, "~");
}

void MachineCommunicationTest::sendSoftResetWhenAskedTo()
{
    auto serialPort = new TestSerialPort();
    TestPortDiscovery portDiscoverer(serialPort);

    MachineCommunication communicator(100);

    QSignalSpy spy(&communicator, &MachineCommunication::dataSent);

    communicator.portFound(MachineInfo("a", "1"), &portDiscoverer);
    communicator.softReset();

    QCOMPARE(spy.count(), 1);
    auto data = spy.at(0).at(0).toByteArray();
    QCOMPARE(data, "\x18");
}

void MachineCommunicationTest::doHardResetWhenAskedTo()
{
    auto serialPort = new TestSerialPort();
    TestPortDiscovery portDiscoverer(serialPort);

    QSignalSpy portOpenedSpy(serialPort, &TestSerialPort::portOpened);
    QSignalSpy portClosedSpy(serialPort, &TestSerialPort::portClosed);

    MachineCommunication communicator(2000);

    QSignalSpy machineInitializedSpy(&communicator, &MachineCommunication::machineInitialized);

    communicator.portFound(MachineInfo("a", "1"), &portDiscoverer);
    QCOMPARE(machineInitializedSpy.count(), 1);

    QElapsedTimer timer;
    timer.start();
    communicator.hardReset();
    const auto elapsed = timer.elapsed();

    QVERIFY(elapsed > 1950);
    QCOMPARE(portOpenedSpy.count(), 1);
    QCOMPARE(portClosedSpy.count(), 1);
    QCOMPARE(machineInitializedSpy.count(), 2);
}

QTEST_GUILESS_MAIN(MachineCommunicationTest)

#include "machinecommunication_test.moc"

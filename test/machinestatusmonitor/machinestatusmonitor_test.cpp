#include <QString>
#include <QtTest>
#include "core/machinestatusmonitor.h"
#include "testcommon/testportdiscovery.h"
#include "testcommon/testserialport.h"
#include "testcommon/utils.h"

class MachineStatusMonitorTest : public QObject
{
    Q_OBJECT

public:
    MachineStatusMonitorTest();

private Q_SLOTS:
    void sendStatusReportQueryCommandWhenMachineIsInitialized();
    void periodicallySendStatusReportQueryCommand();
    // No need to stop polling, attempts to send command when port is closed are discarded
    void returnUnknownStatusAtStart();
    void emitStateChangedSignalIfStatusChangesToIdle();
    void ignoreNonStatusMessages();
    void doNotEmitStateChangedSignalIfStatusDoesNotChange();
    void emitStateChangedSignalIfStatusChangesToOtherStates();
    void resetStateToUnknownWhenPortClosed();
    void resetStateToUnknownWhenPortClosedWithError();
    void resetStateToUnknownWhenMachineIsInitialized();
};

MachineStatusMonitorTest::MachineStatusMonitorTest()
{
}

void MachineStatusMonitorTest::sendStatusReportQueryCommandWhenMachineIsInitialized()
{
    auto serialPort = new TestSerialPort();
    TestPortDiscovery portDiscoverer(serialPort);
    auto communicator = std::make_unique<MachineCommunication>(1000);

    QSignalSpy spy(communicator.get(), &MachineCommunication::dataSent);

    MachineStatusMonitor statusMonitor(1000, communicator.get());

    QCOMPARE(spy.count(), 0);

    communicator->portFound(MachineInfo("a", "1"), &portDiscoverer);

    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(0).toByteArray(), "?");
}

void MachineStatusMonitorTest::periodicallySendStatusReportQueryCommand()
{
    auto serialPort = new TestSerialPort();
    TestPortDiscovery portDiscoverer(serialPort);
    auto communicator = std::make_unique<MachineCommunication>(1000);

    QSignalSpy spy(communicator.get(), &MachineCommunication::dataSent);

    MachineStatusMonitor statusMonitor(500, communicator.get());

    QCOMPARE(spy.count(), 0);

    communicator->portFound(MachineInfo("a", "1"), &portDiscoverer);

    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(0).toByteArray(), "?");

    // More requests at regular intervals
    QVERIFY(!spy.wait(400));
    QVERIFY(spy.wait(200));
    QCOMPARE(spy.count(), 2);
    QCOMPARE(spy.at(1).at(0).toByteArray(), "?");

    QVERIFY(!spy.wait(400));
    QVERIFY(spy.wait(200));
    QCOMPARE(spy.count(), 3);
    QCOMPARE(spy.at(2).at(0).toByteArray(), "?");
}

void MachineStatusMonitorTest::returnUnknownStatusAtStart()
{
    auto communicator = std::move(createCommunicator().first);

    MachineStatusMonitor statusMonitor(500, communicator.get());

    QCOMPARE(statusMonitor.state(), MachineState::Unknown);
}

void MachineStatusMonitorTest::emitStateChangedSignalIfStatusChangesToIdle()
{
    auto communicatorAndPort = createCommunicator();
    auto communicator = std::move(communicatorAndPort.first);
    auto serialPort = communicatorAndPort.second;

    MachineStatusMonitor statusMonitor(500, communicator.get());

    QSignalSpy spy(&statusMonitor, &MachineStatusMonitor::stateChanged);

    serialPort->simulateReceivedData("<Idle|MPos:0.000,0.000,0.000|FS:0,0|WCO:0.000,0.000,0.000>\r\n");

    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(0).value<MachineState>(), MachineState::Idle);
    QCOMPARE(statusMonitor.state(), MachineState::Idle);
}

void MachineStatusMonitorTest::ignoreNonStatusMessages()
{
    auto communicatorAndPort = createCommunicator();
    auto communicator = std::move(communicatorAndPort.first);
    auto serialPort = communicatorAndPort.second;

    MachineStatusMonitor statusMonitor(500, communicator.get());

    QSignalSpy spy(&statusMonitor, &MachineStatusMonitor::stateChanged);

    serialPort->simulateReceivedData("dummy\r\n");

    QCOMPARE(spy.count(), 0);
}

void MachineStatusMonitorTest::doNotEmitStateChangedSignalIfStatusDoesNotChange()
{
    auto communicatorAndPort = createCommunicator();
    auto communicator = std::move(communicatorAndPort.first);
    auto serialPort = communicatorAndPort.second;

    MachineStatusMonitor statusMonitor(500, communicator.get());

    QSignalSpy spy(&statusMonitor, &MachineStatusMonitor::stateChanged);

    serialPort->simulateReceivedData("<Idle|MPos:0.000,0.000,0.000|FS:0,0|WCO:0.000,0.000,0.000>\r\n");

    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(0).value<MachineState>(), MachineState::Idle);

    // Same state no, new signal
    serialPort->simulateReceivedData("<Idle|MPos:0.000,0.000,0.000|FS:0,0|WCO:0.000,0.000,0.000>\r\n");
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(0).value<MachineState>(), MachineState::Idle);
}

void MachineStatusMonitorTest::emitStateChangedSignalIfStatusChangesToOtherStates()
{
    auto communicatorAndPort = createCommunicator();
    auto communicator = std::move(communicatorAndPort.first);
    auto serialPort = communicatorAndPort.second;

    MachineStatusMonitor statusMonitor(500, communicator.get());

    QSignalSpy spy(&statusMonitor, &MachineStatusMonitor::stateChanged);

    serialPort->simulateReceivedData("<Run|MPos:0.000,0.000,0.000|FS:0,0|WCO:0.000,0.000,0.000>\r\n");

    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(0).value<MachineState>(), MachineState::Run);
    QCOMPARE(statusMonitor.state(), MachineState::Run);
}

void MachineStatusMonitorTest::resetStateToUnknownWhenPortClosed()
{
    auto communicatorAndPort = createCommunicator();
    auto communicator = std::move(communicatorAndPort.first);
    auto serialPort = communicatorAndPort.second;

    MachineStatusMonitor statusMonitor(500, communicator.get());

    QSignalSpy spy(&statusMonitor, &MachineStatusMonitor::stateChanged);

    serialPort->simulateReceivedData("<Idle|MPos:0.000,0.000,0.000|FS:0,0|WCO:0.000,0.000,0.000>\r\n");

    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(0).value<MachineState>(), MachineState::Idle);
    QCOMPARE(statusMonitor.state(), MachineState::Idle);

    communicator->closePort();

    QCOMPARE(spy.count(), 2);
    QCOMPARE(spy.at(1).at(0).value<MachineState>(), MachineState::Unknown);
    QCOMPARE(statusMonitor.state(), MachineState::Unknown);
}

void MachineStatusMonitorTest::resetStateToUnknownWhenPortClosedWithError()
{
    auto communicatorAndPort = createCommunicator();
    auto communicator = std::move(communicatorAndPort.first);
    auto serialPort = communicatorAndPort.second;

    MachineStatusMonitor statusMonitor(500, communicator.get());

    QSignalSpy spy(&statusMonitor, &MachineStatusMonitor::stateChanged);

    serialPort->simulateReceivedData("<Idle|MPos:0.000,0.000,0.000|FS:0,0|WCO:0.000,0.000,0.000>\r\n");

    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(0).value<MachineState>(), MachineState::Idle);
    QCOMPARE(statusMonitor.state(), MachineState::Idle);

    communicator->closePortWithError("Error!");

    QCOMPARE(spy.count(), 2);
    QCOMPARE(spy.at(1).at(0).value<MachineState>(), MachineState::Unknown);
    QCOMPARE(statusMonitor.state(), MachineState::Unknown);
}

void MachineStatusMonitorTest::resetStateToUnknownWhenMachineIsInitialized()
{
    auto communicatorAndPort = createCommunicator();
    auto communicator = std::move(communicatorAndPort.first);
    auto serialPort = communicatorAndPort.second;

    MachineStatusMonitor statusMonitor(500, communicator.get());

    QSignalSpy spy(&statusMonitor, &MachineStatusMonitor::stateChanged);

    serialPort->simulateReceivedData("<Idle|MPos:0.000,0.000,0.000|FS:0,0|WCO:0.000,0.000,0.000>\r\n");

    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(0).value<MachineState>(), MachineState::Idle);
    QCOMPARE(statusMonitor.state(), MachineState::Idle);

    communicator->hardReset();

    QCOMPARE(spy.count(), 2);
    QCOMPARE(spy.at(1).at(0).value<MachineState>(), MachineState::Unknown);
    QCOMPARE(statusMonitor.state(), MachineState::Unknown);
}

QTEST_GUILESS_MAIN(MachineStatusMonitorTest)

#include "machinestatusmonitor_test.moc"

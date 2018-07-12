#include <memory>
#include <QBuffer>
#include <QByteArray>
#include <QSignalSpy>
#include <QtTest>
#include "core/commandsender.h"
#include "core/gcodesender.h"
#include "core/machinecommunication.h"
#include "core/machinestatusmonitor.h"
#include "core/wirecontroller.h"
#include "testcommon/testportdiscovery.h"
#include "testcommon/testserialport.h"
#include "testcommon/utils.h"

class TestBuffer : public QBuffer
{
public:
    TestBuffer()
        : QBuffer()
        , m_openError(false)
        , m_readError(false)
    {
    }

    bool open(QIODevice::OpenMode flags) override
    {
        if (m_openError) {
            return false;
        }

        return QBuffer::open(flags);
    }

    void setOpenError(bool openError)
    {
        m_openError = openError;
    }

    void setReadError(bool readError)
    {
        m_readError = readError;
    }

protected:
    qint64 readData(char *data, qint64 maxSize) override
    {
        if (m_readError) {
            return -1;
        }

        return QBuffer::readData(data, maxSize);
    }

private:
    bool m_openError;
    bool m_readError;
};

class GCodeSenderTest : public QObject
{
    Q_OBJECT

    struct Requirements {
        std::unique_ptr<MachineCommunication> communicator;
        TestSerialPort* serialPort;
        std::unique_ptr<CommandSender> commandSender;
        std::unique_ptr<WireController> wireController;
        std::unique_ptr<MachineStatusMonitor> statusMonitor;
    };

public:
    GCodeSenderTest();

private:
    Requirements createRequirements(bool setStateToIdle = true);
    void sendAcks(TestSerialPort *port, int numAcks);
    void sendState(TestSerialPort *port, QByteArray state);

private Q_SLOTS:
    void setGCodeStreamParentToNull();
    void emitSignalWhenStreamingStarts();
    void emitSignalIfGCodeDeviceCouldNotBeOpenedAndDoAHardReset();
    void performAnHardResetAtStart();
    void switchWireOnAfterHardReset();
    void doNothingAfterHardResetIfGCodeStreamIsEmpty();
    void immediatelyEmitStreamEndedIfGCodeStreamIsEmpty();
    void sendSingleCommandAndStopIfGCodeStreamIsMadeUpOfOneCommandOnly();
    void openStreamAsText();
    void endStreamingWithErrorAndSwitchWireOffIfAttemptingToSendAnInvalidCommand();
    void sendAllCommandsAndStopIfGCodeIsShort();
    void stopEnqueingCommandsIfMoreThan10ArePending();
    void doNotEmitStreamingEndedIfThereArePendingCommands();
    void emitStreamingEndedSignalWithErrorAndResetIfItIsNotPossibleToReadTheGCodeStream();
    void doNotStartAfterHardResetIfMachineIsNotIdle();
    void emitStreamingEndedSignalWithSuccessOnlyAfterAllRepliesAreReceivedAndMachineIsIdleAgain();
    void keepWaitingForAcksIfMachineGoesIdlePrematurely();
    void emitStreamingEndedSignalWithErrorAndResetIfMachineRepliesWithError();
    void doNotRestartIfStateGoesFromIdleToAnotherOneNotRunAndThenBackToIdle();
    void emitStreamingEndedSignalWithErrorAndResetIfMachineGoesInUnexpectedState();
    void emitStreamingEndedSignalWithErrorAndResetIfMessageRepliesAreLost();
    void emitStreamingEndedSignalWithErrorAndResetIfUserInterruptStreaming();
};

GCodeSenderTest::GCodeSenderTest()
{
}

GCodeSenderTest::Requirements GCodeSenderTest::createRequirements(bool setStateToIdle)
{
    Requirements r;

    auto communicatorAndPort = createCommunicator();
    r.communicator = std::move(communicatorAndPort.first);
    r.serialPort = communicatorAndPort.second;
    r.commandSender = std::make_unique<CommandSender>(r.communicator.get());
    r.wireController = std::make_unique<WireController>(r.communicator.get(), r.commandSender.get());
    r.statusMonitor = std::make_unique<MachineStatusMonitor>(100000, r.communicator.get());

    if (setStateToIdle) {
        r.serialPort->simulateReceivedData("<Idle|MPos:0.000,0.000,0.000|FS:0,0|WCO:0.000,0.000,0.000>\r\n");
    }

    return r;
}

void GCodeSenderTest::sendAcks(TestSerialPort* port, int numAcks)
{
    for (auto i = 0; i < numAcks; ++i) {
        port->simulateReceivedData("ok\r\n");
    }
}

void GCodeSenderTest::sendState(TestSerialPort *port, QByteArray state)
{
    port->simulateReceivedData("<" + state + "|MPos:0.000,0.000,0.000|FS:0,0|WCO:0.000,0.000,0.000>\r\n");
}

void GCodeSenderTest::setGCodeStreamParentToNull()
{
    auto r = createRequirements();

    auto buffer = new TestBuffer();
    buffer->setParent(this);
    GCodeSender fileSender(r.communicator.get(), r.commandSender.get(), r.wireController.get(), r.statusMonitor.get(), std::unique_ptr<QIODevice>(buffer));

    QCOMPARE(buffer->parent(), nullptr);
}

void GCodeSenderTest::emitSignalWhenStreamingStarts()
{
    auto r = createRequirements();

    auto buffer = new TestBuffer();
    GCodeSender fileSender(r.communicator.get(), r.commandSender.get(), r.wireController.get(), r.statusMonitor.get(), std::unique_ptr<QIODevice>(buffer));

    QSignalSpy spy(&fileSender, &GCodeSender::streamingStarted);

    fileSender.streamData();

    QCOMPARE(spy.count(), 1);
}

void GCodeSenderTest::emitSignalIfGCodeDeviceCouldNotBeOpenedAndDoAHardReset()
{
    auto r = createRequirements();

    auto buffer = new TestBuffer();
    buffer->setOpenError(true);
    GCodeSender fileSender(r.communicator.get(), r.commandSender.get(), r.wireController.get(), r.statusMonitor.get(), std::unique_ptr<QIODevice>(buffer));

    QSignalSpy endSpy(&fileSender, &GCodeSender::streamingEnded);
    QSignalSpy dataSentSpy(r.communicator.get(), &MachineCommunication::dataSent);
    QSignalSpy initializationSpy(r.communicator.get(), &MachineCommunication::machineInitialized);

    fileSender.streamData();

    QCOMPARE(endSpy.count(), 1);
    QCOMPARE(endSpy.at(0).at(0).value<GCodeSender::StreamEndReason>(), GCodeSender::StreamEndReason::StreamError);
    QCOMPARE(endSpy.at(0).at(1).toString(), tr("Input device could not be opened"));
    QCOMPARE(initializationSpy.count(), 1);
}

void GCodeSenderTest::performAnHardResetAtStart()
{
    auto r = createRequirements();

    auto buffer = new TestBuffer();
    GCodeSender fileSender(r.communicator.get(), r.commandSender.get(), r.wireController.get(), r.statusMonitor.get(), std::unique_ptr<QIODevice>(buffer));

    QSignalSpy spy(r.communicator.get(), &MachineCommunication::machineInitialized);

    fileSender.streamData();

    QCOMPARE(spy.count(), 1);
}

void GCodeSenderTest::switchWireOnAfterHardReset()
{
    auto r = createRequirements();

    auto buffer = new TestBuffer();
    buffer->buffer() = "XXX";
    GCodeSender fileSender(r.communicator.get(), r.commandSender.get(), r.wireController.get(), r.statusMonitor.get(), std::unique_ptr<QIODevice>(buffer));

    QSignalSpy spy(r.communicator.get(), &MachineCommunication::dataSent);

    fileSender.streamData();

    QVERIFY(spy.count() >= 5); // First 4 are temeperature set, wire off and status query after hard reset
    QCOMPARE(spy.at(4).at(0).toByteArray(), "M3\n");
}

void GCodeSenderTest::doNothingAfterHardResetIfGCodeStreamIsEmpty()
{
    auto r = createRequirements();

    auto buffer = new TestBuffer();
    GCodeSender fileSender(r.communicator.get(), r.commandSender.get(), r.wireController.get(), r.statusMonitor.get(), std::unique_ptr<QIODevice>(buffer));

    QSignalSpy spy(r.communicator.get(), &MachineCommunication::dataSent);

    fileSender.streamData();

    QCOMPARE(spy.count(), 4); // Does not send wire on and off
}

void GCodeSenderTest::immediatelyEmitStreamEndedIfGCodeStreamIsEmpty()
{
    auto r = createRequirements();

    auto buffer = new TestBuffer();
    GCodeSender fileSender(r.communicator.get(), r.commandSender.get(), r.wireController.get(), r.statusMonitor.get(), std::unique_ptr<QIODevice>(buffer));

    QSignalSpy spy(&fileSender, &GCodeSender::streamingEnded);

    fileSender.streamData();

    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(0).value<GCodeSender::StreamEndReason>(), GCodeSender::StreamEndReason::Completed);
    QCOMPARE(spy.at(0).at(1).toString(), tr("Success"));
}

void GCodeSenderTest::sendSingleCommandAndStopIfGCodeStreamIsMadeUpOfOneCommandOnly()
{
    auto r = createRequirements();

    auto buffer = new TestBuffer();
    buffer->buffer() = "G01 X100\n";
    GCodeSender fileSender(r.communicator.get(), r.commandSender.get(), r.wireController.get(), r.statusMonitor.get(), std::unique_ptr<QIODevice>(buffer));

    QSignalSpy endSpy(&fileSender, &GCodeSender::streamingEnded);
    QSignalSpy dataSentSpy(r.communicator.get(), &MachineCommunication::dataSent);

    fileSender.streamData();

    // Run and then Idle plus acks
    sendState(r.serialPort, "Run");
    sendAcks(r.serialPort, 4);
    sendState(r.serialPort, "Idle");

    QCOMPARE(dataSentSpy.count(), 7); // 5 are inital commands sent after hard reset and the last is wire off
    QCOMPARE(dataSentSpy.at(5).at(0).toByteArray(), "G01 X100\n");

    QCOMPARE(endSpy.count(), 1);
    QCOMPARE(endSpy.at(0).at(0).value<GCodeSender::StreamEndReason>(), GCodeSender::StreamEndReason::Completed);
    QCOMPARE(endSpy.at(0).at(1).toString(), tr("Success"));
}

void GCodeSenderTest::openStreamAsText()
{
    auto r = createRequirements();

    auto buffer = new TestBuffer();
    buffer->buffer() = "G01 X100\r\n"; // This must be converted to \n when stream opened as text
    GCodeSender fileSender(r.communicator.get(), r.commandSender.get(), r.wireController.get(), r.statusMonitor.get(), std::unique_ptr<QIODevice>(buffer));

    QSignalSpy spy(r.communicator.get(), &MachineCommunication::dataSent);

    fileSender.streamData();

    QVERIFY(spy.count() > 5); // 5 are inital commands sent after hard reset and the last is wire off
    QCOMPARE(spy.at(5).at(0).toByteArray(), "G01 X100\n");
}

void GCodeSenderTest::endStreamingWithErrorAndSwitchWireOffIfAttemptingToSendAnInvalidCommand()
{
    auto r = createRequirements();

    auto buffer = new TestBuffer();
    buffer->buffer() = QByteArray(129, 'X'); // Command too long
    GCodeSender fileSender(r.communicator.get(), r.commandSender.get(), r.wireController.get(), r.statusMonitor.get(), std::unique_ptr<QIODevice>(buffer));

    QSignalSpy endSpy(&fileSender, &GCodeSender::streamingEnded);
    QSignalSpy dataSentSpy(r.communicator.get(), &MachineCommunication::dataSent);
    QSignalSpy initializationSpy(r.communicator.get(), &MachineCommunication::machineInitialized);

    fileSender.streamData();

    QVERIFY(dataSentSpy.count() > 5); // 5 are inital commands sent after hard reset and the last is wire off
    QCOMPARE(dataSentSpy.at(5).at(0).toByteArray(), "M5\n");

    QCOMPARE(endSpy.count(), 1);
    QCOMPARE(endSpy.at(0).at(0).value<GCodeSender::StreamEndReason>(), GCodeSender::StreamEndReason::StreamError);
    QCOMPARE(endSpy.at(0).at(1).toString(), tr("Invalid command in GCode stream"));

    QCOMPARE(initializationSpy.count(), 2); // The initial one and the one because of the error
}

void GCodeSenderTest::sendAllCommandsAndStopIfGCodeIsShort()
{
    auto r = createRequirements();

    auto buffer = new TestBuffer();
    buffer->buffer() = "G01 X100\nG01 Y32\nG01 Z123\n";
    GCodeSender fileSender(r.communicator.get(), r.commandSender.get(), r.wireController.get(), r.statusMonitor.get(), std::unique_ptr<QIODevice>(buffer));

    QSignalSpy endSpy(&fileSender, &GCodeSender::streamingEnded);
    QSignalSpy dataSentSpy(r.communicator.get(), &MachineCommunication::dataSent);

    fileSender.streamData();

    // Run and then Idle
    sendState(r.serialPort, "Run");
    sendAcks(r.serialPort, 6);
    sendState(r.serialPort, "Idle");

    QCOMPARE(dataSentSpy.count(), 9); // 5 are inital commands sent after hard reset and the last is wire off
    QCOMPARE(dataSentSpy.at(5).at(0).toByteArray(), "G01 X100\n");
    QCOMPARE(dataSentSpy.at(6).at(0).toByteArray(), "G01 Y32\n");
    QCOMPARE(dataSentSpy.at(7).at(0).toByteArray(), "G01 Z123\n");

    QCOMPARE(endSpy.count(), 1);
    QCOMPARE(endSpy.at(0).at(0).value<GCodeSender::StreamEndReason>(), GCodeSender::StreamEndReason::Completed);
    QCOMPARE(endSpy.at(0).at(1).toString(), tr("Success"));
}

void GCodeSenderTest::stopEnqueingCommandsIfMoreThan10ArePending()
{
    auto r = createRequirements();

    auto buffer = new TestBuffer();
    // 10 bytes (commands after hard reset) + 14 times 8 bytes = 122 bytes, then 12 more commands
    for (auto i = 0; i < 26; ++i) {
        buffer->buffer() += "G01 X10\n";
    }
    GCodeSender fileSender(r.communicator.get(), r.commandSender.get(), r.wireController.get(), r.statusMonitor.get(), std::unique_ptr<QIODevice>(buffer));

    fileSender.streamData();

    QCOMPARE(r.commandSender->pendingCommands(), 11);
}

void GCodeSenderTest::doNotEmitStreamingEndedIfThereArePendingCommands()
{
    auto r = createRequirements();

    auto buffer = new TestBuffer();
    // 10 bytes (commands after hard reset) + 14 times 8 bytes = 122 bytes, then 12 more commands
    for (auto i = 0; i < 26; ++i) {
        buffer->buffer() += "G01 X10\n";
    }
    GCodeSender fileSender(r.communicator.get(), r.commandSender.get(), r.wireController.get(), r.statusMonitor.get(), std::unique_ptr<QIODevice>(buffer));

    QSignalSpy spy(&fileSender, &GCodeSender::streamingEnded);

    fileSender.streamData();

    QCOMPARE(spy.count(), 0);
}

void GCodeSenderTest::emitStreamingEndedSignalWithErrorAndResetIfItIsNotPossibleToReadTheGCodeStream()
{
    auto r = createRequirements();

    auto buffer = new TestBuffer();
    buffer->setReadError(true);
    buffer->buffer() = "XXXXX";
    GCodeSender fileSender(r.communicator.get(), r.commandSender.get(), r.wireController.get(), r.statusMonitor.get(), std::unique_ptr<QIODevice>(buffer));

    QSignalSpy endSpy(&fileSender, &GCodeSender::streamingEnded);
    QSignalSpy initializationSpy(r.communicator.get(), &MachineCommunication::machineInitialized);

    fileSender.streamData();

    QCOMPARE(endSpy.count(), 1);
    QCOMPARE(endSpy.at(0).at(0).value<GCodeSender::StreamEndReason>(), GCodeSender::StreamEndReason::StreamError);
    QCOMPARE(endSpy.at(0).at(1).toString(), tr("Could not read GCode line from input device"));

    QCOMPARE(initializationSpy.count(), 2); // The initial one and the one because of the error
}

void GCodeSenderTest::doNotStartAfterHardResetIfMachineIsNotIdle()
{
    auto r = createRequirements(false);

    auto buffer = new TestBuffer();
    buffer->buffer() = "G01 X100\n";
    GCodeSender fileSender(r.communicator.get(), r.commandSender.get(), r.wireController.get(), r.statusMonitor.get(), std::unique_ptr<QIODevice>(buffer));

    QSignalSpy endSpy(&fileSender, &GCodeSender::streamingEnded);
    QSignalSpy dataSentSpy(r.communicator.get(), &MachineCommunication::dataSent);

    fileSender.streamData();

    QCOMPARE(dataSentSpy.count(), 4); // commands sent after hard reset

    // Now machine goes Idle and streaming starts
    sendState(r.serialPort, "Idle");

    QCOMPARE(dataSentSpy.count(), 6);
    QCOMPARE(dataSentSpy.at(4).at(0).toByteArray(), "M3\n");
    QCOMPARE(dataSentSpy.at(5).at(0).toByteArray(), "G01 X100\n");
}

void GCodeSenderTest::emitStreamingEndedSignalWithSuccessOnlyAfterAllRepliesAreReceivedAndMachineIsIdleAgain()
{
    auto r = createRequirements();

    auto buffer = new TestBuffer();
    buffer->buffer() = "G01 X100\n";
    GCodeSender fileSender(r.communicator.get(), r.commandSender.get(), r.wireController.get(), r.statusMonitor.get(), std::unique_ptr<QIODevice>(buffer));

    QSignalSpy endSpy(&fileSender, &GCodeSender::streamingEnded);
    QSignalSpy dataSentSpy(r.communicator.get(), &MachineCommunication::dataSent);

    fileSender.streamData();

    // Now machine goes in Run state
    sendState(r.serialPort, "Run");

    QCOMPARE(dataSentSpy.count(), 6);

    // No end signal
    QCOMPARE(endSpy.count(), 0);

    // Replies
    sendAcks(r.serialPort, 4);

    // No end signal, waiting to go idle
    QCOMPARE(endSpy.count(), 0);

    // Now machine goes Idle
    sendState(r.serialPort, "Idle");

    QCOMPARE(endSpy.count(), 1);
    QCOMPARE(endSpy.at(0).at(0).value<GCodeSender::StreamEndReason>(), GCodeSender::StreamEndReason::Completed);
    QCOMPARE(endSpy.at(0).at(1).toString(), tr("Success"));
}

void GCodeSenderTest::keepWaitingForAcksIfMachineGoesIdlePrematurely()
{
    auto r = createRequirements();

    auto buffer = new TestBuffer();
    buffer->buffer() = "G01 X100\n";
    GCodeSender fileSender(r.communicator.get(), r.commandSender.get(), r.wireController.get(), r.statusMonitor.get(), std::unique_ptr<QIODevice>(buffer));

    QSignalSpy endSpy(&fileSender, &GCodeSender::streamingEnded);
    QSignalSpy dataSentSpy(r.communicator.get(), &MachineCommunication::dataSent);

    fileSender.streamData();

    // Now machine goes in Run state
    sendState(r.serialPort, "Run");

    QCOMPARE(dataSentSpy.count(), 6);

    // No end signal
    QCOMPARE(endSpy.count(), 0);

    // Now machine goes Idle before acks, we have to wait
    sendState(r.serialPort, "Idle");

    // No end signal
    QCOMPARE(endSpy.count(), 0);

    // Replies
    sendAcks(r.serialPort, 4);

    QCOMPARE(endSpy.count(), 1);
    QCOMPARE(endSpy.at(0).at(0).value<GCodeSender::StreamEndReason>(), GCodeSender::StreamEndReason::Completed);
    QCOMPARE(endSpy.at(0).at(1).toString(), tr("Success"));
}

void GCodeSenderTest::emitStreamingEndedSignalWithErrorAndResetIfMachineRepliesWithError()
{
    auto r = createRequirements();

    auto buffer = new TestBuffer();
    buffer->buffer() = "XXXXX";
    GCodeSender fileSender(r.communicator.get(), r.commandSender.get(), r.wireController.get(), r.statusMonitor.get(), std::unique_ptr<QIODevice>(buffer));

    QSignalSpy endSpy(&fileSender, &GCodeSender::streamingEnded);
    QSignalSpy initializationSpy(r.communicator.get(), &MachineCommunication::machineInitialized);

    fileSender.streamData();

    // acks for initial commands, then error for our command
    sendAcks(r.serialPort, 3);
    r.serialPort->simulateReceivedData("error:17\r\n");

    QCOMPARE(endSpy.count(), 1);
    QCOMPARE(endSpy.at(0).at(0).value<GCodeSender::StreamEndReason>(), GCodeSender::StreamEndReason::MachineError);
    QCOMPARE(endSpy.at(0).at(1).toString(), tr("Firmware replied with error:") + "17");

    QCOMPARE(initializationSpy.count(), 2); // The initial one and the one because of the error
}

void GCodeSenderTest::doNotRestartIfStateGoesFromIdleToAnotherOneNotRunAndThenBackToIdle()
{
    auto r = createRequirements();

    auto buffer = new TestBuffer();
    buffer->buffer() = "XXXXX";
    GCodeSender fileSender(r.communicator.get(), r.commandSender.get(), r.wireController.get(), r.statusMonitor.get(), std::unique_ptr<QIODevice>(buffer));

    QSignalSpy endSpy(&fileSender, &GCodeSender::streamingEnded);
    QSignalSpy initializationSpy(r.communicator.get(), &MachineCommunication::machineInitialized);

    fileSender.streamData();

    // We are idle, change to Hold then to Idle again, nothing should happend
    sendState(r.serialPort, "Hold");
    sendState(r.serialPort, "Idle");

    QCOMPARE(endSpy.count(), 0);
}

void GCodeSenderTest::emitStreamingEndedSignalWithErrorAndResetIfMachineGoesInUnexpectedState()
{
    auto r = createRequirements();

    auto buffer = new TestBuffer();
    buffer->buffer() = "XXXXX";
    GCodeSender fileSender(r.communicator.get(), r.commandSender.get(), r.wireController.get(), r.statusMonitor.get(), std::unique_ptr<QIODevice>(buffer));

    QSignalSpy endSpy(&fileSender, &GCodeSender::streamingEnded);
    QSignalSpy initializationSpy(r.communicator.get(), &MachineCommunication::machineInitialized);

    fileSender.streamData();

    // Change first to allowed states, then to another state and expect streaming to end. We are Idle,
    // start again with Unknown then the others
    sendState(r.serialPort, "Unknown");
    sendState(r.serialPort, "Idle");
    sendState(r.serialPort, "Run");
    sendState(r.serialPort, "Hold");
    sendState(r.serialPort, "Alarm"); // This one is unexpected

    QCOMPARE(endSpy.count(), 1);
    QCOMPARE(endSpy.at(0).at(0).value<GCodeSender::StreamEndReason>(), GCodeSender::StreamEndReason::MachineError);
    QCOMPARE(endSpy.at(0).at(1).toString(), tr("Machine changed to unexpected state: ") + "Alarm");

    QCOMPARE(initializationSpy.count(), 2); // The initial one and the one because of the error
}

void GCodeSenderTest::emitStreamingEndedSignalWithErrorAndResetIfMessageRepliesAreLost()
{
    auto r = createRequirements();

    auto buffer = new TestBuffer();
    buffer->buffer() = "XXXXX";
    GCodeSender fileSender(r.communicator.get(), r.commandSender.get(), r.wireController.get(), r.statusMonitor.get(), std::unique_ptr<QIODevice>(buffer));

    QSignalSpy endSpy(&fileSender, &GCodeSender::streamingEnded);
    QSignalSpy initializationSpy(r.communicator.get(), &MachineCommunication::machineInitialized);

    fileSender.streamData();

    // Closing the port is a way to cause message replies to get lost
    r.communicator->closePort();

    QCOMPARE(endSpy.count(), 1);
    QCOMPARE(endSpy.at(0).at(0).value<GCodeSender::StreamEndReason>(), GCodeSender::StreamEndReason::PortError);
    QCOMPARE(endSpy.at(0).at(1).toString(), tr("Failed to get replies for some commands"));

    QCOMPARE(initializationSpy.count(), 2); // The initial one and the one because of the error
}

void GCodeSenderTest::emitStreamingEndedSignalWithErrorAndResetIfUserInterruptStreaming()
{
    auto r = createRequirements();

    auto buffer = new TestBuffer();
    buffer->buffer() = "XXXXX";
    GCodeSender fileSender(r.communicator.get(), r.commandSender.get(), r.wireController.get(), r.statusMonitor.get(), std::unique_ptr<QIODevice>(buffer));

    QSignalSpy endSpy(&fileSender, &GCodeSender::streamingEnded);
    QSignalSpy initializationSpy(r.communicator.get(), &MachineCommunication::machineInitialized);

    fileSender.streamData();

    // Closing the port is a way to cause message replies to get lost
    fileSender.interruptStreaming();

    QCOMPARE(endSpy.count(), 1);
    QCOMPARE(endSpy.at(0).at(0).value<GCodeSender::StreamEndReason>(), GCodeSender::StreamEndReason::UserInterrupted);
    QCOMPARE(endSpy.at(0).at(1).toString(), tr("User interrupted streaming"));

    QCOMPARE(initializationSpy.count(), 2); // The initial one and the one because of the error
}

QTEST_GUILESS_MAIN(GCodeSenderTest)

#include "gcodesender_test.moc"

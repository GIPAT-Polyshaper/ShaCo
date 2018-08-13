#include <memory>
#include <vector>
#include <QByteArray>
#include <QList>
#include <QPair>
#include <QSignalSpy>
#include <QtTest>
#include "core/commandsender.h"
#include "core/machinecommunication.h"
#include "testcommon/testmachineinfo.h"
#include "testcommon/testportdiscovery.h"
#include "testcommon/testserialport.h"
#include "testcommon/utils.h"

class TestCommandReplyListener : public CommandSenderListener
{
public:
    using ErrorPair = QPair<CommandCorrelationId, int>;
    using LostPair = QPair<CommandCorrelationId, bool>;

    void commandSent(CommandCorrelationId correlationId) override
    {
        m_sentCalls.append(correlationId);
    }

    void okReply(CommandCorrelationId correlationId) override
    {
        m_okCalls.append(correlationId);
    }

    void errorReply(CommandCorrelationId correlationId, int errorCode) override
    {
        m_errorCalls.append(ErrorPair(correlationId, errorCode));
    }

    void replyLost(CommandCorrelationId correlationId, bool commandSent) override
    {
        m_lostCalls.append(LostPair(correlationId, commandSent));
    }

    const QList<CommandCorrelationId>& sentCalls() const
    {
        return m_sentCalls;
    }

    const QList<CommandCorrelationId>& okCalls() const
    {
        return m_okCalls;
    }

    const QList<ErrorPair>& errorCalls() const
    {
        return m_errorCalls;
    }

    const QList<LostPair>& lostCalls() const
    {
        return m_lostCalls;
    }

private:
    QList<CommandCorrelationId> m_sentCalls;
    QList<CommandCorrelationId> m_okCalls;
    QList<ErrorPair> m_errorCalls;
    QList<LostPair> m_lostCalls;
};

class CommandSenderTest : public QObject
{
    Q_OBJECT

public:
    CommandSenderTest();

private:
    TestMachineInfo m_info;

private Q_SLOTS:
    void return0PendingCommandsAtStart();
    void returnFalseIfAttemptingToSendCommandLongerThan127BytesWithNoEndline();
    void returnTrueIfAttemptingToSendCommand128BytesLongWithEndline();
    void sendCommandWhenRequested();
    void appendEndlineIfMissing();
    void returnFalseIfCommandHasEndlineInTheMiddle();
    void returnFalseIfCarriageReturnIsPresent();
    void callOkReplyOfListenerWhenOkIsReceived();
    void callErroReplyOfListenerWhenErrorIsReceived();
    void ignoreUnknownMessages();
    void callTheRightListenerWhenMessageIsReceived();
    void doNotCallListenerIfNullWhenCommandSucceeds();
    void doNotCallListenerIfNullWhenCommandFails();
    void ignoreUnexpectedOkMessages();
    void ignoreUnexpectedErrorMessages();
    void doNotSendOkNotificationsToDeletedListeners();
    void doNotSendErrorNotificationsToDeletedListeners();
    void doNotSendMoreThan128BytesWithoutAReply();
    void sendCommandThatExceeds128BytesAfterReplyReceived();
    void returnTheNumberOfPendingCommands();
    void doNotSentCommandIfItsSizeCausesSentBytesToExceed128();
    void sendAsManyEnqueuedCommandsAsPossibleAfterAReply();
    void sendQueuedCommandsWhenReplyIsError();
    void resetSentMessagesWhenPortClosed();
    void resetToSendMessagesWhenPortClosed();
    void callReplyLostOfListenersWhenPortClosed();
    void doNotCallReplyLostOfNullAndDeletedListenersWhenPortClosed();
    void resetStateWhenPortClosedWithError();
    void resetStateWhenMachineInitialized();
    void neverSendNewCommandsIfTheareAreEnqueuedOnes();
    void callCommandSentWhenACommandIsSent();
    void doNotCallCommandSentOfListenerIfListerWasDeleted();
    void discardNestedCallsToResetState();
};

CommandSenderTest::CommandSenderTest()
{
}

void CommandSenderTest::return0PendingCommandsAtStart()
{
    auto communicator = std::move(createCommunicator(&m_info).first);
    CommandSender sender(communicator.get());

    QCOMPARE(sender.pendingCommands(), 0);
}

void CommandSenderTest::returnFalseIfAttemptingToSendCommandLongerThan127BytesWithNoEndline()
{
    auto communicator = std::move(createCommunicator(&m_info).first);
    CommandSender sender(communicator.get());

    QVERIFY(!sender.sendCommand(QByteArray(128, 'X')));
}

void CommandSenderTest::returnTrueIfAttemptingToSendCommand128BytesLongWithEndline()
{
    auto communicator = std::move(createCommunicator(&m_info).first);
    CommandSender sender(communicator.get());

    QByteArray command(128, 'X');
    command[127] = '\n';
    QVERIFY(sender.sendCommand(command));
}

void CommandSenderTest::sendCommandWhenRequested()
{
    auto communicator = std::move(createCommunicator(&m_info).first);
    CommandSender sender(communicator.get());

    QSignalSpy spy(communicator.get(), &MachineCommunication::dataSent);

    QVERIFY(sender.sendCommand("Pippo\n"));

    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(0).toByteArray(), "Pippo\n");
}

void CommandSenderTest::appendEndlineIfMissing()
{
    auto communicator = std::move(createCommunicator(&m_info).first);
    CommandSender sender(communicator.get());

    QSignalSpy spy(communicator.get(), &MachineCommunication::dataSent);

    QVERIFY(sender.sendCommand("Pippo"));

    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(0).toByteArray(), "Pippo\n");
}

void CommandSenderTest::returnFalseIfCommandHasEndlineInTheMiddle()
{
    auto communicator = std::move(createCommunicator(&m_info).first);
    CommandSender sender(communicator.get());

    QSignalSpy spy(communicator.get(), &MachineCommunication::dataSent);

    QVERIFY(!sender.sendCommand("Pi\nppo\n"));

    QCOMPARE(spy.count(), 0);
}

void CommandSenderTest::returnFalseIfCarriageReturnIsPresent()
{
    auto communicator = std::move(createCommunicator(&m_info).first);
    CommandSender sender(communicator.get());

    QSignalSpy spy(communicator.get(), &MachineCommunication::dataSent);

    QVERIFY(!sender.sendCommand("Pi\rppo\n"));

    QCOMPARE(spy.count(), 0);
}

void CommandSenderTest::callOkReplyOfListenerWhenOkIsReceived()
{
    auto communicatorAndPort = createCommunicator(&m_info);
    auto communicator = std::move(communicatorAndPort.first);
    auto serialPort = communicatorAndPort.second;
    CommandSender sender(communicator.get());

    TestCommandReplyListener listener;

    QVERIFY(sender.sendCommand("Pippo", 17, &listener));

    serialPort->simulateReceivedData("ok\r\n");

    QCOMPARE(listener.okCalls().size(), 1);
    QCOMPARE(listener.okCalls()[0], 17u);
}

void CommandSenderTest::callErroReplyOfListenerWhenErrorIsReceived()
{
    auto communicatorAndPort = createCommunicator(&m_info);
    auto communicator = std::move(communicatorAndPort.first);
    auto serialPort = communicatorAndPort.second;
    CommandSender sender(communicator.get());

    TestCommandReplyListener listener;

    QVERIFY(sender.sendCommand("Pippo", 13, &listener));

    serialPort->simulateReceivedData("error:78\r\n");

    QCOMPARE(listener.okCalls().size(), 0);
    QCOMPARE(listener.errorCalls().size(), 1);
    QCOMPARE(listener.errorCalls()[0].first, 13u);
    QCOMPARE(listener.errorCalls()[0].second, 78);
}

void CommandSenderTest::ignoreUnknownMessages()
{
    auto communicatorAndPort = createCommunicator(&m_info);
    auto communicator = std::move(communicatorAndPort.first);
    auto serialPort = communicatorAndPort.second;
    CommandSender sender(communicator.get());

    TestCommandReplyListener listener;

    QVERIFY(sender.sendCommand("Pippo", 13, &listener));

    serialPort->simulateReceivedData("dummy\r\n");

    QCOMPARE(listener.okCalls().size(), 0);
    QCOMPARE(listener.errorCalls().size(), 0);
}

void CommandSenderTest::callTheRightListenerWhenMessageIsReceived()
{
    auto communicatorAndPort = createCommunicator(&m_info);
    auto communicator = std::move(communicatorAndPort.first);
    auto serialPort = communicatorAndPort.second;
    CommandSender sender(communicator.get());

    TestCommandReplyListener listener1;
    TestCommandReplyListener listener2;

    QVERIFY(sender.sendCommand("Pippo", 17, &listener1));
    QVERIFY(sender.sendCommand("Pippo", 13, &listener2));

    serialPort->simulateReceivedData("ok\r\n");

    QCOMPARE(listener1.errorCalls().size(), 0);
    QCOMPARE(listener1.okCalls().size(), 1);
    QCOMPARE(listener1.okCalls()[0], 17u);

    serialPort->simulateReceivedData("error:54\r\n");

    QCOMPARE(listener2.okCalls().size(), 0);
    QCOMPARE(listener2.errorCalls().size(), 1);
    QCOMPARE(listener2.errorCalls()[0].first, 13u);
    QCOMPARE(listener2.errorCalls()[0].second, 54);
}

void CommandSenderTest::doNotCallListenerIfNullWhenCommandSucceeds()
{
    auto communicatorAndPort = createCommunicator(&m_info);
    auto communicator = std::move(communicatorAndPort.first);
    auto serialPort = communicatorAndPort.second;
    CommandSender sender(communicator.get());

    QVERIFY(sender.sendCommand("Pippo"));

    // This should simply not crash
    serialPort->simulateReceivedData("ok\r\n");
}

void CommandSenderTest::doNotCallListenerIfNullWhenCommandFails()
{
    auto communicatorAndPort = createCommunicator(&m_info);
    auto communicator = std::move(communicatorAndPort.first);
    auto serialPort = communicatorAndPort.second;
    CommandSender sender(communicator.get());

    QVERIFY(sender.sendCommand("Pippo"));

    // This should simply not crash
    serialPort->simulateReceivedData("error:43\r\n");
}

void CommandSenderTest::ignoreUnexpectedOkMessages()
{
    auto communicatorAndPort = createCommunicator(&m_info);
    auto communicator = std::move(communicatorAndPort.first);
    auto serialPort = communicatorAndPort.second;
    CommandSender sender(communicator.get());

    // This should simply not crash
    serialPort->simulateReceivedData("ok\r\n");
}

void CommandSenderTest::ignoreUnexpectedErrorMessages()
{
    auto communicatorAndPort = createCommunicator(&m_info);
    auto communicator = std::move(communicatorAndPort.first);
    auto serialPort = communicatorAndPort.second;
    CommandSender sender(communicator.get());

    // This should simply not crash
    serialPort->simulateReceivedData("error:97\r\n");
}

void CommandSenderTest::doNotSendOkNotificationsToDeletedListeners()
{
    auto communicatorAndPort = createCommunicator(&m_info);
    auto communicator = std::move(communicatorAndPort.first);
    auto serialPort = communicatorAndPort.second;
    CommandSender sender(communicator.get());

    auto listener = std::make_unique<TestCommandReplyListener>();

    QVERIFY(sender.sendCommand("Pippo", 17, listener.get()));

    listener.reset();

    // This should simply not crash
    serialPort->simulateReceivedData("ok\r\n");
}

void CommandSenderTest::doNotSendErrorNotificationsToDeletedListeners()
{
    auto communicatorAndPort = createCommunicator(&m_info);
    auto communicator = std::move(communicatorAndPort.first);
    auto serialPort = communicatorAndPort.second;
    CommandSender sender(communicator.get());

    auto listener = std::make_unique<TestCommandReplyListener>();

    QVERIFY(sender.sendCommand("Pippo", 17, listener.get()));

    listener.reset();

    // This should simply not crash
    serialPort->simulateReceivedData("error:75\r\n");
}

void CommandSenderTest::doNotSendMoreThan128BytesWithoutAReply()
{
    auto communicator = std::move(createCommunicator(&m_info).first);
    CommandSender sender(communicator.get());

    QSignalSpy spy(communicator.get(), &MachineCommunication::dataSent);

    // Sending 16 time 8 bytes = 128 bytes, then some more bytes
    for (auto i = 0; i < 16; ++i) {
        sender.sendCommand("0123456\n");
    }
    QVERIFY(sender.sendCommand("more\n"));

    // Should only receive notifications of the first 16 lines
    QCOMPARE(spy.count(), 16);
    for (auto i = 0; i < 16; ++i) {
        QCOMPARE(spy.at(i).at(0).toByteArray(), "0123456\n");
    }
}

void CommandSenderTest::sendCommandThatExceeds128BytesAfterReplyReceived()
{
    auto communicatorAndPort = createCommunicator(&m_info);
    auto communicator = std::move(communicatorAndPort.first);
    auto serialPort = communicatorAndPort.second;
    CommandSender sender(communicator.get());

    QSignalSpy spy(communicator.get(), &MachineCommunication::dataSent);

    // Sending 16 time 8 bytes = 128 bytes, then some more bytes
    for (auto i = 0; i < 16; ++i) {
        sender.sendCommand("0123456\n");
    }
    QVERIFY(sender.sendCommand("more\n"));

    QCOMPARE(spy.count(), 16);

    // Now replying to one line
    serialPort->simulateReceivedData("ok\r\n");

    // Another line sent
    QCOMPARE(spy.count(), 17);
    QCOMPARE(spy.at(16).at(0).toByteArray(), "more\n");
}

void CommandSenderTest::returnTheNumberOfPendingCommands()
{
    auto communicator = std::move(createCommunicator(&m_info).first);
    CommandSender sender(communicator.get());

    // Sending 16 time 8 bytes = 128 bytes, then some more commands
    for (auto i = 0; i < 16; ++i) {
        sender.sendCommand("0123456\n");
    }
    sender.sendCommand("more\n");
    sender.sendCommand("commands\n");
    sender.sendCommand("to send\n");

    QCOMPARE(sender.pendingCommands(), 3);
}

void CommandSenderTest::doNotSentCommandIfItsSizeCausesSentBytesToExceed128()
{
    auto communicator = std::move(createCommunicator(&m_info).first);
    CommandSender sender(communicator.get());

    QSignalSpy spy(communicator.get(), &MachineCommunication::dataSent);

    // Sending 15 time 8 bytes = 120 bytes, then a command longer than 8 bytes (that is not sent)
    for (auto i = 0; i < 15; ++i) {
        sender.sendCommand("0123456\n");
    }
    QVERIFY(sender.sendCommand("01234567\n"));

    QCOMPARE(spy.count(), 15);
}

void CommandSenderTest::sendAsManyEnqueuedCommandsAsPossibleAfterAReply()
{
    auto communicatorAndPort = createCommunicator(&m_info);
    auto communicator = std::move(communicatorAndPort.first);
    auto serialPort = communicatorAndPort.second;
    CommandSender sender(communicator.get());

    QSignalSpy spy(communicator.get(), &MachineCommunication::dataSent);

    // Sending 16 time 8 bytes = 128 bytes, then some more bytes
    for (auto i = 0; i < 16; ++i) {
        sender.sendCommand("0123456\n");
    }
    QVERIFY(sender.sendCommand("01\n"));
    QVERIFY(sender.sendCommand("34\n"));
    QVERIFY(sender.sendCommand("6\n"));
    QVERIFY(sender.sendCommand("89\n")); // This is not sent after the first reply

    QCOMPARE(spy.count(), 16);

    // Now replying to one line
    serialPort->simulateReceivedData("ok\r\n");

    // Three more lines sent
    QCOMPARE(spy.count(), 19);
    QCOMPARE(spy.at(16).at(0).toByteArray(), "01\n");
    QCOMPARE(spy.at(17).at(0).toByteArray(), "34\n");
    QCOMPARE(spy.at(18).at(0).toByteArray(), "6\n");
}

void CommandSenderTest::sendQueuedCommandsWhenReplyIsError()
{
    auto communicatorAndPort = createCommunicator(&m_info);
    auto communicator = std::move(communicatorAndPort.first);
    auto serialPort = communicatorAndPort.second;
    CommandSender sender(communicator.get());

    QSignalSpy spy(communicator.get(), &MachineCommunication::dataSent);

    // Sending 16 time 8 bytes = 128 bytes, then some more bytes
    for (auto i = 0; i < 16; ++i) {
        sender.sendCommand("0123456\n");
    }
    QVERIFY(sender.sendCommand("01\n"));
    QVERIFY(sender.sendCommand("34\n"));
    QVERIFY(sender.sendCommand("6\n"));
    QVERIFY(sender.sendCommand("89\n")); // This is not sent after the first reply

    QCOMPARE(spy.count(), 16);

    // Now replying to one line
    serialPort->simulateReceivedData("error:71\r\n");

    // Three more lines sent
    QCOMPARE(spy.count(), 19);
    QCOMPARE(spy.at(16).at(0).toByteArray(), "01\n");
    QCOMPARE(spy.at(17).at(0).toByteArray(), "34\n");
    QCOMPARE(spy.at(18).at(0).toByteArray(), "6\n");
}

void CommandSenderTest::resetSentMessagesWhenPortClosed()
{
    auto communicator = std::move(createCommunicator(&m_info).first);
    CommandSender sender(communicator.get());

    QSignalSpy spy(communicator.get(), &MachineCommunication::dataSent);

    // Sending 16 time 8 bytes = 128 bytes, then some more bytes
    for (auto i = 0; i < 16; ++i) {
        sender.sendCommand("0123456\n");
    }
    sender.sendCommand("more\n");

    // Should only receive notifications of the first 16 lines
    QCOMPARE(spy.count(), 16);

    // Close and re-open port
    communicator->closePort();
    auto serialPort = new TestSerialPort();
    TestPortDiscovery portDiscoverer(serialPort);
    communicator->portFound(&m_info, &portDiscoverer);

    // New commands should be sent immediately and old one discarded
    sender.sendCommand("abc\n");

    QCOMPARE(spy.count(), 17);
    QCOMPARE(spy.at(16).at(0).toByteArray(), "abc\n");
}

void CommandSenderTest::resetToSendMessagesWhenPortClosed()
{
    auto communicator = std::move(createCommunicator(&m_info).first);
    CommandSender sender(communicator.get());

    QSignalSpy spy(communicator.get(), &MachineCommunication::dataSent);

    // Sending 16 time 8 bytes = 128 bytes, then some more bytes
    for (auto i = 0; i < 16; ++i) {
        sender.sendCommand("0123456\n");
    }
    sender.sendCommand("more\n");

    // Should only receive notifications of the first 16 lines
    QCOMPARE(spy.count(), 16);

    // Close and re-open port
    communicator->closePort();
    auto serialPort = new TestSerialPort();
    TestPortDiscovery portDiscoverer(serialPort);
    communicator->portFound(&m_info, &portDiscoverer);

    // Send new commands and up to 128 bytes and then one more
    for (auto i = 0; i < 16; ++i) {
        sender.sendCommand("abcdefg\n");
    }
    sender.sendCommand("new one\n");

    QCOMPARE(spy.count(), 32);

    // send one reply and expect one more command sent (the new one)
    serialPort->simulateReceivedData("ok\r\n");
    QCOMPARE(spy.count(), 33);
    QCOMPARE(spy.at(32).at(0).toByteArray(), "new one\n");
}

void CommandSenderTest::callReplyLostOfListenersWhenPortClosed()
{
    auto communicator = std::move(createCommunicator(&m_info).first);
    CommandSender sender(communicator.get());

    // Create listeners
    std::vector<std::unique_ptr<TestCommandReplyListener>> listeners;
    for (auto i = 0; i < 17; ++i) {
        listeners.push_back(std::make_unique<TestCommandReplyListener>());
    }

    // Sending 16 time 8 bytes = 128 bytes, then some more bytes
    for (auto i = 0u; i < 16u; ++i) {
        sender.sendCommand("0123456\n", i, listeners[i].get());
    }
    sender.sendCommand("more\n", 16, listeners[16].get());

    // Closing port, all listeners should be called
    communicator->closePort();
    for (auto i = 0u; i < 16u; ++i) {
        QCOMPARE(listeners[i]->lostCalls().count(), 1);
        QCOMPARE(listeners[i]->lostCalls()[0].first, i);
        QCOMPARE(listeners[i]->lostCalls()[0].second, true);
    }
    QCOMPARE(listeners[16]->lostCalls().count(), 1);
    QCOMPARE(listeners[16]->lostCalls()[0].first, 16u);
    QCOMPARE(listeners[16]->lostCalls()[0].second, false);
}

void CommandSenderTest::doNotCallReplyLostOfNullAndDeletedListenersWhenPortClosed()
{
    auto communicator = std::move(createCommunicator(&m_info).first);
    CommandSender sender(communicator.get());

    // Create listeners
    std::vector<std::unique_ptr<TestCommandReplyListener>> listeners;
    for (auto i = 0; i < 18; ++i) {
        listeners.push_back(std::make_unique<TestCommandReplyListener>());
    }

    // Setting some listeners to null, non-null ones are deleted below
    for (auto i = 0u; i < 8u; ++i) {
        sender.sendCommand("0123456\n", i, listeners[i].get());
    }
    for (auto i = 8u; i < 16u; ++i) {
        sender.sendCommand("0123456\n");
    }
    sender.sendCommand("more\n", 16, listeners[16].get());
    sender.sendCommand("one more\n");

    // Deleting all listeners
    listeners.clear();

    // Closing port, this should simply not crash
    communicator->closePort();
}

void CommandSenderTest::resetStateWhenPortClosedWithError()
{
    // This tests that closePortWithError has the same effect of closePort (all in one test)

    auto communicator = std::move(createCommunicator(&m_info).first);
    CommandSender sender(communicator.get());

    QSignalSpy spy(communicator.get(), &MachineCommunication::dataSent);

    // Create listeners, some empty some full
    std::vector<std::unique_ptr<TestCommandReplyListener>> listeners;
    for (auto i = 0; i < 19; ++i) {
        if (i % 3 == 0) {
            listeners.push_back(std::unique_ptr<TestCommandReplyListener>());
        } else {
            listeners.push_back(std::make_unique<TestCommandReplyListener>());
        }
    }

    // Sending 16 time 8 bytes = 128 bytes, then some more bytes
    for (auto i = 0u; i < 16u; ++i) {
        sender.sendCommand("0123456\n", i, listeners[i].get());
    }
    sender.sendCommand("more\n", 16, listeners[16].get());
    sender.sendCommand("one more\n", 17, listeners[17].get());
    sender.sendCommand("again\n", 18, listeners[18].get());

    // remove some listeners
    for (auto i = 0u; i < 19u; ++i) {
        if (i % 3 == 1) {
            listeners[i].reset();
        }
    }

    // Should only receive notifications of the first 16 lines
    QCOMPARE(spy.count(), 16);

    // Close and re-open port
    communicator->closePortWithError("bla");
    auto serialPort = new TestSerialPort();
    TestPortDiscovery portDiscoverer(serialPort);
    communicator->portFound(&m_info, &portDiscoverer);

    // Send new commands and up to 128 bytes and then one more
    for (auto i = 0; i < 16; ++i) {
        sender.sendCommand("abcdefg\n");
    }
    sender.sendCommand("new one\n");

    QCOMPARE(spy.count(), 32);

    // send one reply and expect one more command sent (the new one)
    serialPort->simulateReceivedData("ok\r\n");
    QCOMPARE(spy.count(), 33);
    QCOMPARE(spy.at(32).at(0).toByteArray(), "new one\n");

    // Non-null listeners are called
    for (auto i = 0u; i < 19u; ++i) {
        if (listeners[i]){
            QCOMPARE(listeners[i]->lostCalls().count(), 1);
            QCOMPARE(listeners[i]->lostCalls()[0].first, i);
            if (i < 16u) {
                QCOMPARE(listeners[i]->lostCalls()[0].second, true);
            } else {
                QCOMPARE(listeners[i]->lostCalls()[0].second, false);
            }
        }
    }
}

void CommandSenderTest::resetStateWhenMachineInitialized()
{
    // This tests that machine reset has the same effect of closePort (all in one test)

    auto communicatorAndPort = createCommunicator(&m_info);
    auto communicator = std::move(communicatorAndPort.first);
    auto serialPort = communicatorAndPort.second;
    CommandSender sender(communicator.get());

    QSignalSpy spy(communicator.get(), &MachineCommunication::dataSent);

    // Create listeners, some empty some full
    std::vector<std::unique_ptr<TestCommandReplyListener>> listeners;
    for (auto i = 0; i < 19; ++i) {
        if (i % 3 == 0) {
            listeners.push_back(std::unique_ptr<TestCommandReplyListener>());
        } else {
            listeners.push_back(std::make_unique<TestCommandReplyListener>());
        }
    }

    // Sending 16 time 8 bytes = 128 bytes, then some more bytes
    for (auto i = 0u; i < 16u; ++i) {
        sender.sendCommand("0123456\n", i, listeners[i].get());
    }
    sender.sendCommand("more\n", 16, listeners[16].get());
    sender.sendCommand("one more\n", 17, listeners[17].get());
    sender.sendCommand("again\n", 18, listeners[18].get());

    // remove some listeners
    for (auto i = 0u; i < 19u; ++i) {
        if (i % 3 == 1) {
            listeners[i].reset();
        }
    }

    // Should only receive notifications of the first 16 lines
    QCOMPARE(spy.count(), 16);

    // Hard reset
    communicator->hardReset();

    // Send new commands and up to 128 bytes and then one more
    for (auto i = 0; i < 16; ++i) {
        sender.sendCommand("abcdefg\n");
    }
    sender.sendCommand("new one\n");

    QCOMPARE(spy.count(), 33); // index 16 is the hard reset command

    // send one reply and expect one more command sent (the new one)
    serialPort->simulateReceivedData("ok\r\n");
    QCOMPARE(spy.count(), 34);
    QCOMPARE(spy.at(33).at(0).toByteArray(), "new one\n");

    // Non-null listeners are called
    for (auto i = 0u; i < 19u; ++i) {
        if (listeners[i]){
            QCOMPARE(listeners[i]->lostCalls().count(), 1);
            QCOMPARE(listeners[i]->lostCalls()[0].first, i);
            if (i < 16u) {
                QCOMPARE(listeners[i]->lostCalls()[0].second, true);
            } else {
                QCOMPARE(listeners[i]->lostCalls()[0].second, false);
            }
        }
    }
}

void CommandSenderTest::neverSendNewCommandsIfTheareAreEnqueuedOnes()
{
    auto communicator = std::move(createCommunicator(&m_info).first);
    CommandSender sender(communicator.get());

    QSignalSpy spy(communicator.get(), &MachineCommunication::dataSent);

    // Sending 15 time 8 bytes = 120 bytes, then a command longer than 8 bytes (that is not sent)
    // and then another short command that must not be sent
    for (auto i = 0u; i < 15u; ++i) {
        sender.sendCommand("0123456\n");
    }
    QVERIFY(sender.sendCommand("01234567\n"));
    QVERIFY(sender.sendCommand("0123\n"));

    QCOMPARE(spy.count(), 15);
    QCOMPARE(sender.pendingCommands(), 2);
}

void CommandSenderTest::callCommandSentWhenACommandIsSent()
{
    auto communicator = std::move(createCommunicator(&m_info).first);
    CommandSender sender(communicator.get());

    TestCommandReplyListener listener;

    QVERIFY(sender.sendCommand("Pippo", 17, &listener));

    QCOMPARE(listener.sentCalls().size(), 1);
    QCOMPARE(listener.sentCalls()[0], 17u);
}

void CommandSenderTest::doNotCallCommandSentOfListenerIfListerWasDeleted()
{
    auto communicatorAndPort = createCommunicator(&m_info);
    auto communicator = std::move(communicatorAndPort.first);
    auto serialPort = communicatorAndPort.second;
    CommandSender sender(communicator.get());

    auto listener = std::make_unique<TestCommandReplyListener>();

    QSignalSpy spy(communicator.get(), &MachineCommunication::dataSent);

    // Sending 16 time 8 bytes = 128 bytes, then some more bytes
    for (auto i = 0u; i < 16u; ++i) {
        sender.sendCommand("0123456\n");
    }
    QVERIFY(sender.sendCommand("abcde\n", 123, listener.get()));

    listener.reset();

    // Now replying to one line. This should simply not crash
    serialPort->simulateReceivedData("ok\r\n");
}

void CommandSenderTest::discardNestedCallsToResetState()
{
    // This was a bug: calling hardReset from replyLost() callback caused
    // nested execution of resetState(), with container being modified while
    // iterators were active (in the outer call)
    class ListenerWithReset : public CommandSenderListener
    {
    public:
        ListenerWithReset(MachineCommunication* c) : m_communicator(c) {}

        void commandSent(CommandCorrelationId) override {}
        void okReply(CommandCorrelationId) override {}
        void errorReply(CommandCorrelationId, int) override {}
        void replyLost(CommandCorrelationId, bool) override
        {
            m_communicator->hardReset();
        }

    private:
        MachineCommunication* m_communicator;
    };

    auto communicator = std::move(createCommunicator(&m_info).first);
    CommandSender sender(communicator.get());

    auto listener1 = std::make_unique<ListenerWithReset>(communicator.get());
    auto listener2 = std::make_unique<ListenerWithReset>(communicator.get());

    sender.sendCommand("0123456\n", 0, listener1.get());
    sender.sendCommand("0123456\n", 0, listener2.get());

    // This should simply not crash
    communicator->closePortWithError("bla bla bla");
}

QTEST_GUILESS_MAIN(CommandSenderTest)

#include "commandsender_test.moc"

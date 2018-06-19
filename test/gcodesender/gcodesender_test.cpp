#include <functional>
#include <memory>
#include <QString>
#include <QBuffer>
#include <QByteArray>
#include <QSignalSpy>
#include <QTimer>
#include <QtTest>
#include "core/gcodesender.h"
#include "core/machinecommunication.h"
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

public:
    GCodeSenderTest();

private:
    void sendAcks(QTimer& timer, TestSerialPort* serialPort, int numAcks);

private Q_SLOTS:
    void setGCodeStreamParentToNull();
    void sendSignalWhenStreamingStartsAndEnds();
    void deleteDeviceWhenStreamingEnds();
    void sendStreamingEndSignalWithPortErrorReasonInCaseOfError();
    void sendStreamingEndSignalWithStreamErrorReasonIfInputStreamCannotBeOpened();
    void sendStreamingEndSignalWithStreamErrorReasonIfInputStreamCannotBeRead();
    void addEndlineToStreamedDataIfMissing();
    void sendMultipleLines();
    void sendStreamingEndSignalWithPortClosedReasonIfPortClosed();
    void sendStreamingEndSignalWithStreamInterruptedIfUserInterrupts();
    void sendStreamingEndSignalWithStreamErrorReasonIfFirmwareRepliesWithError();
    void correctlyHandleReplyWhenSplittedInMultipleChunks();
    void correctlyHandleMultipleRepliesAtOnce();
    void ignoreUnexpectedFirmwareResponses();
    void doNotSendMoreThan128BytesWithoutAReply();
    void doNotSendLineIfItWouldCauseMoreThan128BytesToBeSent();
    void allowPausingAndResumingStreaming();
    void allowInterruptingStreamingAlsoWhileInPause();
    void doNotSendWireOffIfItWouldCauseMoreThan128BytesToBeSent();
    void doNotExpectAcksForWireControllerRealTimeCommands();
};

GCodeSenderTest::GCodeSenderTest()
{
}

void GCodeSenderTest::sendAcks(QTimer& timer, TestSerialPort* serialPort, int numAcks)
{
    int ackCount = 0;
    connect(&timer, &QTimer::timeout, [serialPort, &ackCount, numAcks](){
        if (ackCount < numAcks) {
            serialPort->simulateReceivedData("ok\r\n");
        }
        ++ackCount;
    });
    timer.start();
}

void GCodeSenderTest::setGCodeStreamParentToNull()
{
    auto communicatorAndPort = createCommunicator();
    auto communicator = std::move(communicatorAndPort.first);
    WireController wireController(communicator.get());

    auto bufferUptr = std::make_unique<TestBuffer>();
    bufferUptr->setParent(this);
    bufferUptr->buffer() = "G01 X100\n";
    TestBuffer* const buffer = bufferUptr.get();
    GCodeSender fileSender(communicator.get(), &wireController, std::move(bufferUptr));

    QCOMPARE(buffer->parent(), nullptr);
}

void GCodeSenderTest::sendSignalWhenStreamingStartsAndEnds()
{
    auto communicatorAndPort = createCommunicator();
    auto communicator = std::move(communicatorAndPort.first);
    auto serialPort = communicatorAndPort.second;
    WireController wireController(communicator.get());

    auto buffer = std::make_unique<TestBuffer>();
    buffer->buffer() = "G01 X100\n";
    GCodeSender fileSender(communicator.get(), &wireController, std::move(buffer));

    QSignalSpy dataSentSpy(communicator.get(), &MachineCommunication::dataSent);
    QSignalSpy startSpy(&fileSender, &GCodeSender::streamingStarted);
    QSignalSpy endSpy(&fileSender, &GCodeSender::streamingEnded);

    // This is used to schedule a function to be executed when the QT event loop is executed
    // (interval is 0).
    QTimer timer;
    int ackCount = 0;
    connect(&timer, &QTimer::timeout, [serialPort, &endSpy, &ackCount](){
        QCOMPARE(endSpy.count(), 0);
        if (ackCount < 3) {
            serialPort->simulateReceivedData("ok\r\n");
        }
        ++ackCount;
    });
    timer.start();

    fileSender.streamData();

    QCOMPARE(dataSentSpy.count(), 3);
    QCOMPARE(dataSentSpy.at(0).at(0).toByteArray(), "M3\n");
    QCOMPARE(dataSentSpy.at(1).at(0).toByteArray(), "G01 X100\n");
    QCOMPARE(dataSentSpy.at(2).at(0).toByteArray(), "M5\n");
    QCOMPARE(startSpy.count(), 1);
    QCOMPARE(endSpy.count(), 1);
    auto reason = endSpy.at(0).at(0).value<GCodeSender::StreamEndReason>();
    auto description = endSpy.at(0).at(1).toString();
    QCOMPARE(reason, GCodeSender::StreamEndReason::Completed);
    QCOMPARE(description, tr("Success"));
}

void GCodeSenderTest::deleteDeviceWhenStreamingEnds()
{
    auto communicatorAndPort = createCommunicator();
    auto communicator = std::move(communicatorAndPort.first);
    auto serialPort = communicatorAndPort.second;
    WireController wireController(communicator.get());

    auto bufferUptr = std::make_unique<TestBuffer>();
    bufferUptr->buffer() = "G01 X100\n";
    TestBuffer* const buffer = bufferUptr.get();
    GCodeSender fileSender(communicator.get(), &wireController, std::move(bufferUptr));

    QSignalSpy deviceDeleted(buffer, &QIODevice::destroyed);

    // This is used to schedule a function to be executed when the QT event loop is executed
    // (interval is 0).
    QTimer timer;
    sendAcks(timer, serialPort, 3);

    fileSender.streamData();

    QCOMPARE(deviceDeleted.count(), 1);
}

void GCodeSenderTest::sendStreamingEndSignalWithPortErrorReasonInCaseOfError()
{
    auto communicatorAndPort = createCommunicator();
    auto communicator = std::move(communicatorAndPort.first);
    communicatorAndPort.second->setInError(true);
    WireController wireController(communicator.get());

    auto bufferUptr = std::make_unique<TestBuffer>();
    bufferUptr->buffer() = "G01 X100\n";
    TestBuffer* const buffer = bufferUptr.get();
    GCodeSender fileSender(communicator.get(), &wireController, std::move(bufferUptr));

    QSignalSpy deviceDeleted(buffer, &QIODevice::destroyed);

    QSignalSpy endSpy(&fileSender, &GCodeSender::streamingEnded);

    fileSender.streamData();

    QCOMPARE(endSpy.count(), 1);
    auto reason = endSpy.at(0).at(0).value<GCodeSender::StreamEndReason>();
    auto description = endSpy.at(0).at(1).toString();
    QCOMPARE(reason, GCodeSender::StreamEndReason::PortError);
    QCOMPARE(deviceDeleted.count(), 1);
    QCOMPARE(description, tr("Serial port closed with error"));
}

void GCodeSenderTest::sendStreamingEndSignalWithStreamErrorReasonIfInputStreamCannotBeOpened()
{
    auto communicator = createCommunicator().first;
    WireController wireController(communicator.get());

    auto bufferUptr = std::make_unique<TestBuffer>();
    bufferUptr->setOpenError(true);
    bufferUptr->buffer() = "G01 X100\n";
    TestBuffer* const buffer = bufferUptr.get();
    GCodeSender fileSender(communicator.get(), &wireController, std::move(bufferUptr));

    QSignalSpy deviceDeleted(buffer, &QIODevice::destroyed);

    QSignalSpy endSpy(&fileSender, &GCodeSender::streamingEnded);

    fileSender.streamData();

    QCOMPARE(endSpy.count(), 1);
    auto reason = endSpy.at(0).at(0).value<GCodeSender::StreamEndReason>();
    auto description = endSpy.at(0).at(1).toString();
    QCOMPARE(reason, GCodeSender::StreamEndReason::StreamError);
    QCOMPARE(deviceDeleted.count(), 1);
    QCOMPARE(description, tr("Input device could not be opened"));
}

void GCodeSenderTest::sendStreamingEndSignalWithStreamErrorReasonIfInputStreamCannotBeRead()
{
    auto communicator = createCommunicator().first;
    WireController wireController(communicator.get());

    auto bufferUptr = std::make_unique<TestBuffer>();
    bufferUptr->setReadError(true);
    bufferUptr->buffer() = "G01 X100\n";
    TestBuffer* const buffer = bufferUptr.get();
    GCodeSender fileSender(communicator.get(), &wireController, std::move(bufferUptr));

    QSignalSpy deviceDeleted(buffer, &QIODevice::destroyed);

    QSignalSpy endSpy(&fileSender, &GCodeSender::streamingEnded);

    fileSender.streamData();

    QCOMPARE(endSpy.count(), 1);
    auto reason = endSpy.at(0).at(0).value<GCodeSender::StreamEndReason>();
    auto description = endSpy.at(0).at(1).toString();
    QCOMPARE(reason, GCodeSender::StreamEndReason::StreamError);
    QCOMPARE(deviceDeleted.count(), 1);
    QCOMPARE(description, tr("Could not read GCode line from input device"));
}

void GCodeSenderTest::addEndlineToStreamedDataIfMissing()
{
    auto communicatorAndPort = createCommunicator();
    auto communicator = std::move(communicatorAndPort.first);
    auto serialPort = communicatorAndPort.second;
    WireController wireController(communicator.get());

    QSignalSpy spy(communicator.get(), &MachineCommunication::dataSent);

    auto buffer = std::make_unique<TestBuffer>();
    buffer->buffer() = "G01 X100";
    GCodeSender fileSender(communicator.get(), &wireController, std::move(buffer));

    // This is used to schedule a function to be executed when the QT event loop is executed
    // (interval is 0).
    QTimer timer;
    sendAcks(timer, serialPort, 3);

    fileSender.streamData();

    QCOMPARE(spy.count(), 3);
    auto data = spy.at(1).at(0).toByteArray();
    QCOMPARE(data, "G01 X100\n");
}

void GCodeSenderTest::sendMultipleLines()
{
    auto communicatorAndPort = createCommunicator();
    auto communicator = std::move(communicatorAndPort.first);
    auto serialPort = communicatorAndPort.second;
    WireController wireController(communicator.get());

    QSignalSpy spy(communicator.get(), &MachineCommunication::dataSent);

    auto buffer = std::make_unique<TestBuffer>();
    buffer->buffer() = "G01 X100\nG01 Y1\nG00 Z9";
    GCodeSender fileSender(communicator.get(), &wireController, std::move(buffer));

    QSignalSpy endSpy(&fileSender, &GCodeSender::streamingEnded);

    // This is used to schedule a function to be executed when the QT event loop is executed
    // (interval is 0).
    QTimer timer;
    sendAcks(timer, serialPort, 5);

    fileSender.streamData();

    QCOMPARE(spy.count(), 5); // 3 + 2 (wire on and off)
    QCOMPARE(spy.at(1).at(0).toByteArray(), "G01 X100\n");
    QCOMPARE(spy.at(2).at(0).toByteArray(), "G01 Y1\n");
    QCOMPARE(spy.at(3).at(0).toByteArray(), "G00 Z9\n");
    QCOMPARE(endSpy.count(), 1);
    auto reason = endSpy.at(0).at(0).value<GCodeSender::StreamEndReason>();
    QCOMPARE(reason, GCodeSender::StreamEndReason::Completed);
}

void GCodeSenderTest::sendStreamingEndSignalWithPortClosedReasonIfPortClosed()
{
    auto communicator = createCommunicator().first;
    WireController wireController(communicator.get());

    // This is used to schedule a function to be executed when the QT event loop is executed
    // (interval is 0)
    QTimer timer;
    connect(&timer, &QTimer::timeout, [communicator = communicator.get()](){ communicator->closePort(); });
    timer.start();

    auto bufferUptr = std::make_unique<TestBuffer>();
    bufferUptr->buffer() = "G01 X100\nG01 Y1\nG00 Z9";
    TestBuffer* const buffer = bufferUptr.get();
    GCodeSender fileSender(communicator.get(), &wireController, std::move(bufferUptr));

    QSignalSpy deviceDeleted(buffer, &QIODevice::destroyed);

    QSignalSpy endSpy(&fileSender, &GCodeSender::streamingEnded);

    fileSender.streamData();

    QCOMPARE(endSpy.count(), 1);
    auto reason = endSpy.at(0).at(0).value<GCodeSender::StreamEndReason>();
    auto description = endSpy.at(0).at(1).toString();
    QCOMPARE(reason, GCodeSender::StreamEndReason::PortClosed);
    QCOMPARE(deviceDeleted.count(), 1);
    QCOMPARE(description, tr("Serial port closed"));
}

void GCodeSenderTest::sendStreamingEndSignalWithStreamInterruptedIfUserInterrupts()
{
    auto communicator = createCommunicator().first;
    WireController wireController(communicator.get());

    auto bufferUptr = std::make_unique<TestBuffer>();
    bufferUptr->buffer() = "G01 X100\nG01 Y1\nG00 Z9";
    TestBuffer* const buffer = bufferUptr.get();
    GCodeSender fileSender(communicator.get(), &wireController, std::move(bufferUptr));

    QSignalSpy deviceDeleted(buffer, &QIODevice::destroyed);

    // This is used to schedule a function to be executed when the QT event loop is executed
    // (interval is 0)
    QTimer timer;
    connect(&timer, &QTimer::timeout, [&fileSender](){ fileSender.interruptStreaming(); });
    timer.start();

    QSignalSpy endSpy(&fileSender, &GCodeSender::streamingEnded);

    fileSender.streamData();

    QCOMPARE(endSpy.count(), 1);
    auto reason = endSpy.at(0).at(0).value<GCodeSender::StreamEndReason>();
    auto description = endSpy.at(0).at(1).toString();
    QCOMPARE(reason, GCodeSender::StreamEndReason::UserInterrupted);
    QCOMPARE(description, tr("Streaming interrupted by the user"));
    QCOMPARE(deviceDeleted.count(), 1);
}

void GCodeSenderTest::sendStreamingEndSignalWithStreamErrorReasonIfFirmwareRepliesWithError()
{
    auto communicatorAndPort = createCommunicator();
    auto communicator = std::move(communicatorAndPort.first);
    auto serialPort = communicatorAndPort.second;
    WireController wireController(communicator.get());

    auto bufferUptr = std::make_unique<TestBuffer>();
    bufferUptr->buffer() = "G01 X100\nG01 Y1\nG00 Z9";
    TestBuffer* const buffer = bufferUptr.get();
    GCodeSender fileSender(communicator.get(), &wireController, std::move(bufferUptr));

    QSignalSpy deviceDeleted(buffer, &QIODevice::destroyed);

    // This is used to schedule a function to be executed when the QT event loop is executed
    // (interval is 0)
    QTimer timer;
    connect(&timer, &QTimer::timeout, [serialPort](){ serialPort->simulateReceivedData("error:10\r\n"); });
    timer.start();

    QSignalSpy endSpy(&fileSender, &GCodeSender::streamingEnded);

    fileSender.streamData();

    QCOMPARE(endSpy.count(), 1);
    auto reason = endSpy.at(0).at(0).value<GCodeSender::StreamEndReason>();
    auto description = endSpy.at(0).at(1).toString();
    QCOMPARE(reason, GCodeSender::StreamEndReason::MachineErrorReply);
    QCOMPARE(description, tr("Firmware replied with error: ") + "10");
    QCOMPARE(deviceDeleted.count(), 1);
}

void GCodeSenderTest::correctlyHandleReplyWhenSplittedInMultipleChunks()
{
    auto communicatorAndPort = createCommunicator();
    auto communicator = std::move(communicatorAndPort.first);
    auto serialPort = communicatorAndPort.second;
    WireController wireController(communicator.get());

    auto bufferUptr = std::make_unique<TestBuffer>();
    bufferUptr->buffer() = "G01 X100\nG01 Y1\nG00 Z9";
    TestBuffer* const buffer = bufferUptr.get();
    GCodeSender fileSender(communicator.get(), &wireController, std::move(bufferUptr));

    QSignalSpy deviceDeleted(buffer, &QIODevice::destroyed);

    // This is used to schedule a function to be executed when the QT event loop is executed
    // (interval is 0)
    QTimer timer;
    int count = 0;
    connect(&timer, &QTimer::timeout, [serialPort, &count](){
        // The second line causes and error and error message is received in two parts
        if (count == 1) {
            serialPort->simulateReceivedData("erro");
        } else if (count == 2) {
            serialPort->simulateReceivedData("r:10\r\n");
        } else {
            serialPort->simulateReceivedData("ok\r\n");
        }
        ++count;
    });
    timer.start();

    QSignalSpy endSpy(&fileSender, &GCodeSender::streamingEnded);

    fileSender.streamData();

    QCOMPARE(endSpy.count(), 1);
    auto reason = endSpy.at(0).at(0).value<GCodeSender::StreamEndReason>();
    QCOMPARE(reason, GCodeSender::StreamEndReason::MachineErrorReply);
    QCOMPARE(deviceDeleted.count(), 1);
}

void GCodeSenderTest::correctlyHandleMultipleRepliesAtOnce()
{
    auto communicatorAndPort = createCommunicator();
    auto communicator = std::move(communicatorAndPort.first);
    auto serialPort = communicatorAndPort.second;
    WireController wireController(communicator.get());

    auto buffer = std::make_unique<TestBuffer>();
    buffer->buffer() = "G01 X100\nG01 Y1\nG00 Z9";
    GCodeSender fileSender(communicator.get(), &wireController, std::move(buffer));

    // This is used to schedule a function to be executed when the QT event loop is executed
    // (interval is 0)
    QTimer timer;
    int count = 0;
    connect(&timer, &QTimer::timeout, [serialPort, &count](){
        if (count == 0) {
            serialPort->simulateReceivedData("o");
        } else if (count == 1) {
            serialPort->simulateReceivedData("k\r\nok\r\n");
        } else if (count < 5) {
            serialPort->simulateReceivedData("ok\r\n");
        }
        ++count;
    });
    timer.start();

    QSignalSpy endSpy(&fileSender, &GCodeSender::streamingEnded);

    fileSender.streamData();

    QCOMPARE(endSpy.count(), 1);
    auto reason = endSpy.at(0).at(0).value<GCodeSender::StreamEndReason>();
    QCOMPARE(reason, GCodeSender::StreamEndReason::Completed);
}

void GCodeSenderTest::ignoreUnexpectedFirmwareResponses()
{
    auto communicatorAndPort = createCommunicator();
    auto communicator = std::move(communicatorAndPort.first);
    auto serialPort = communicatorAndPort.second;
    WireController wireController(communicator.get());

    auto buffer = std::make_unique<TestBuffer>();
    buffer->buffer() = "G01 X100\nG01 Y1\nG00 Z9";
    GCodeSender fileSender(communicator.get(), &wireController, std::move(buffer));

    // This is used to schedule a function to be executed when the QT event loop is executed
    // (interval is 0)
    QTimer timer;
    int count = 0;
    int okSent = 0;
    connect(&timer, &QTimer::timeout, [serialPort, &count, &okSent](){
        if (count == 1) {
            serialPort->simulateReceivedData("unexpected\r\n");
        } else if (okSent < 5) {
            serialPort->simulateReceivedData("ok\r\n");
            ++okSent;
        }
        ++count;
    });
    timer.start();

    QSignalSpy endSpy(&fileSender, &GCodeSender::streamingEnded);

    fileSender.streamData();

    QCOMPARE(endSpy.count(), 1);
    QCOMPARE(okSent, 5);
}

class GrblBufferLimitTestHelper : public QObject
{
    Q_OBJECT

public:
    GrblBufferLimitTestHelper(int limitByte, MachineCommunication* communicator, TestSerialPort* serialPort)
        : m_limitByte(limitByte)
        , m_serialPort(serialPort)
        , m_bytesSent(0)
        , m_pauseCyclesAtLimitByte(0)
        , m_expectStartAgainSendingData(false)
        , m_startedAgainSendingData(true)
        , m_acksToSend(0)
    {
        connect(communicator, &MachineCommunication::dataSent, this, &GrblBufferLimitTestHelper::dataSent);
        connect(&m_timer, &QTimer::timeout, this, &GrblBufferLimitTestHelper::timeout);
        m_timer.start();
    }

    int pauseCyclesAtLimitByte() const
    {
        return m_pauseCyclesAtLimitByte;
    }

    bool startedAgainSendingData() const
    {
        return m_startedAgainSendingData;
    }

public slots:
    void dataSent(QByteArray data)
    {
        m_bytesSent += data.size();
        ++m_acksToSend;
    }

    void timeout()
    {
        if (m_bytesSent < m_limitByte) {
            return;
        } else if (m_bytesSent == m_limitByte && m_pauseCyclesAtLimitByte < 3) {
            ++m_pauseCyclesAtLimitByte;
        } else {
            if (m_expectStartAgainSendingData) {
                // This becomes true only if GCodeSender has sent more data right after the first ack
                m_startedAgainSendingData = true;
                m_expectStartAgainSendingData = false;
            }
            if (m_acksToSend > 0) {
                m_serialPort->simulateReceivedData("ok\r\n");
                --m_acksToSend;
            }
            m_expectStartAgainSendingData = true;
        }
    }

private:
    QTimer m_timer;
    // At what byte sending should stop. This can be less than 128 if sending another line would
    // cause the 128 limit to be crossed
    const int m_limitByte;
    TestSerialPort* const m_serialPort;
    int m_bytesSent;
    int m_pauseCyclesAtLimitByte;
    bool m_expectStartAgainSendingData;
    bool m_startedAgainSendingData;
    int m_acksToSend;
};

void GCodeSenderTest::doNotSendMoreThan128BytesWithoutAReply()
{
    auto communicatorAndPort = createCommunicator();
    auto communicator = std::move(communicatorAndPort.first);
    auto serialPort = communicatorAndPort.second;
    WireController wireController(communicator.get());

    auto buffer = std::make_unique<TestBuffer>();
    // 3 bytes (wire on) + 5 bytes + 15 times 8 bytes = 128 bytes. Then 3 more instructions - total 19 times 8 bytes
    buffer->buffer() += "G01 \n";
    for (auto i = 0; i < 18; ++i) {
        buffer->buffer() += "G01 X10\n";
    }
    GCodeSender fileSender(communicator.get(), &wireController, std::move(buffer));

    GrblBufferLimitTestHelper testHelper(128, communicator.get(), serialPort);

    fileSender.streamData();

    QCOMPARE(testHelper.pauseCyclesAtLimitByte(), 3);
    QVERIFY(testHelper.startedAgainSendingData());
}

void GCodeSenderTest::doNotSendLineIfItWouldCauseMoreThan128BytesToBeSent()
{
    auto communicatorAndPort = createCommunicator();
    auto communicator = std::move(communicatorAndPort.first);
    auto serialPort = communicatorAndPort.second;
    WireController wireController(communicator.get());

    auto buffer = std::make_unique<TestBuffer>();
    // 3 bytes (wire on) + 15 times 8 bytes = 123 bytes
    for (auto i = 0; i < 15; ++i) {
        buffer->buffer() += "G01 X10\n";
    }
    // Now 4 bytes -> 127 bytes
    buffer->buffer() += "G01\n";
    // Next line would cause the 128 bytes limit to be crossed, should wait before sending
    buffer->buffer() += "G01 X10\n";
    GCodeSender fileSender(communicator.get(), &wireController, std::move(buffer));

    GrblBufferLimitTestHelper testHelper(127, communicator.get(), serialPort);

    fileSender.streamData();

    QCOMPARE(testHelper.pauseCyclesAtLimitByte(), 3);
    QVERIFY(testHelper.startedAgainSendingData());
}

void GCodeSenderTest::allowPausingAndResumingStreaming()
{
    auto communicatorAndPort = createCommunicator();
    auto communicator = std::move(communicatorAndPort.first);
    auto serialPort = communicatorAndPort.second;
    WireController wireController(communicator.get());

    QSignalSpy dataSentSpy(communicator.get(), &MachineCommunication::dataSent);

    auto buffer = std::make_unique<TestBuffer>();
    buffer->buffer() = "G01 Y1\nG00 Z9\n";
    GCodeSender fileSender(communicator.get(), &wireController, std::move(buffer));

    QSignalSpy endSpy(&fileSender, &GCodeSender::streamingEnded);
    QSignalSpy pausedSpy(&fileSender, &GCodeSender::streamingPaused);
    QSignalSpy resumedSpy(&fileSender, &GCodeSender::streamingResumed);

    // This is used to schedule a function to be executed when the QT event loop is executed
    // (interval is 0).
    QTimer timer;
    int count = 0;
    // All the conditions that will be verified at the end (QCOMPARE and QVERIFY inside the closure
    // do not work). They must all evaluate to true at the end of this test
    bool pausedFalseAtBegin = false;
    bool pausedSignalSent = false;
    bool firstDataSent = false;
    bool pauseWhilePausedDoesNotSendSignal = false;
    bool noDataSentAfterPause = false;
    bool pausedStaysTrueWhilePaused = false;
    bool pausedBecomesFalseAfterResume = false;
    bool resumedSignalSend = false;
    bool resumeWhileResumedDoesNotSendSignal = false;
    bool dataRestartsBeingSentAfterResume = false;
    bool lastCommandSent = false;
    connect(&timer, &QTimer::timeout, [&](){
        if (count == 0) {
            serialPort->simulateReceivedData("ok\r\n");
            serialPort->simulateReceivedData("ok\r\n");
            pausedFalseAtBegin = !fileSender.paused();
            fileSender.pause();
            pausedSignalSent = (pausedSpy.count() == 1);
            firstDataSent = (dataSentSpy.count() == 2); // wire on + first instruction
            noDataSentAfterPause = true;
            pausedStaysTrueWhilePaused = fileSender.paused();
            pauseWhilePausedDoesNotSendSignal = true;
        } else if (count < 4) {
            fileSender.pause();
            pauseWhilePausedDoesNotSendSignal = (pauseWhilePausedDoesNotSendSignal && pausedSpy.count() == 1);
            noDataSentAfterPause = (noDataSentAfterPause && dataSentSpy.count() == 2);
            pausedStaysTrueWhilePaused = (pausedStaysTrueWhilePaused && fileSender.paused());
        } else if (count == 4) {
            fileSender.resume();
            pausedBecomesFalseAfterResume = !fileSender.paused();
            resumedSignalSend = (resumedSpy.count() == 1);
            resumeWhileResumedDoesNotSendSignal = true;
        } else if (dataSentSpy.count() == 3){
            serialPort->simulateReceivedData("ok\r\n");
            fileSender.resume();
            resumeWhileResumedDoesNotSendSignal = (resumeWhileResumedDoesNotSendSignal && resumedSpy.count() == 1);
            dataRestartsBeingSentAfterResume = true;
            pausedBecomesFalseAfterResume = (pausedBecomesFalseAfterResume && !fileSender.paused());
        } else if (dataSentSpy.count() == 4 && !lastCommandSent){
            serialPort->simulateReceivedData("ok\r\n");
            lastCommandSent = true;
        }
        ++count;
    });
    timer.start();

    fileSender.streamData();

    QCOMPARE(endSpy.count(), 1);
    auto reason = endSpy.at(0).at(0).value<GCodeSender::StreamEndReason>();
    QCOMPARE(reason, GCodeSender::StreamEndReason::Completed);

    QVERIFY(pausedFalseAtBegin);
    QVERIFY(pausedSignalSent);
    QVERIFY(firstDataSent);
    QVERIFY(pauseWhilePausedDoesNotSendSignal);
    QVERIFY(noDataSentAfterPause);
    QVERIFY(pausedStaysTrueWhilePaused);
    QVERIFY(pausedBecomesFalseAfterResume);
    QVERIFY(resumedSignalSend);
    QVERIFY(resumeWhileResumedDoesNotSendSignal);
    QVERIFY(dataRestartsBeingSentAfterResume);
}

void GCodeSenderTest::allowInterruptingStreamingAlsoWhileInPause()
{
    auto communicatorAndPort = createCommunicator();
    auto communicator = std::move(communicatorAndPort.first);
    WireController wireController(communicator.get());

    auto buffer = std::make_unique<TestBuffer>();
    buffer->buffer() = "G01 Y1\nG00 Z9\n";
    GCodeSender fileSender(communicator.get(), &wireController, std::move(buffer));

    QSignalSpy endSpy(&fileSender, &GCodeSender::streamingEnded);

    // This is used to schedule a function to be executed when the QT event loop is executed
    // (interval is 0).
    QTimer timer;
    int count = 0;
    connect(&timer, &QTimer::timeout, [&count, &fileSender](){
        if (count == 0) {
            fileSender.pause();
        } else if (count == 3) {
            fileSender.interruptStreaming();
        }
        ++count;
    });
    timer.start();

    fileSender.streamData();

    QCOMPARE(endSpy.count(), 1);
    auto reason = endSpy.at(0).at(0).value<GCodeSender::StreamEndReason>();
    QCOMPARE(reason, GCodeSender::StreamEndReason::UserInterrupted);
}

void GCodeSenderTest::doNotSendWireOffIfItWouldCauseMoreThan128BytesToBeSent()
{
    auto communicatorAndPort = createCommunicator();
    auto communicator = std::move(communicatorAndPort.first);
    auto serialPort = communicatorAndPort.second;
    WireController wireController(communicator.get());

    auto buffer = std::make_unique<TestBuffer>();
    // 3 bytes (wire on) + 15 times 8 bytes = 123 bytes
    for (auto i = 0; i < 15; ++i) {
        buffer->buffer() += "G01 X10\n";
    }
    // Now 4 bytes -> 127 bytes
    buffer->buffer() += "G01\n";
    GCodeSender fileSender(communicator.get(), &wireController, std::move(buffer));

    GrblBufferLimitTestHelper testHelper(127, communicator.get(), serialPort);

    fileSender.streamData();

    QCOMPARE(testHelper.pauseCyclesAtLimitByte(), 3);
    QVERIFY(testHelper.startedAgainSendingData());
}

void GCodeSenderTest::doNotExpectAcksForWireControllerRealTimeCommands()
{
    // This is to avoid regressions (we used to count bytes from dataSent signal of
    // MachineCommunication, which is wrong)
    auto communicatorAndPort = createCommunicator();
    auto communicator = std::move(communicatorAndPort.first);
    auto serialPort = communicatorAndPort.second;
    WireController wireController(communicator.get());

    auto buffer = std::make_unique<TestBuffer>();
    buffer->buffer() = "G01 Y1\nG00 Z9\n";
    GCodeSender fileSender(communicator.get(), &wireController, std::move(buffer));

    QSignalSpy endSpy(&fileSender, &GCodeSender::streamingEnded);

    QTimer timer;
    int count = 0;
    connect(&timer, &QTimer::timeout, [&count, serialPort, &wireController](){
        if (count < 4) {
            serialPort->simulateReceivedData("ok\r\n");
        }
        if (count == 1) {
            // This does not receive an ack
            wireController.setRealTimeTemperature(11.3);
        }
        ++count;
    });
    timer.start();

    fileSender.streamData();

    QCOMPARE(endSpy.count(), 1);
    auto reason = endSpy.at(0).at(0).value<GCodeSender::StreamEndReason>();
    QCOMPARE(reason, GCodeSender::StreamEndReason::Completed);
}

QTEST_GUILESS_MAIN(GCodeSenderTest)

#include "gcodesender_test.moc"
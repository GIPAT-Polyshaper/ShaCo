#include <functional>
#include <memory>
#include <QString>
#include <QBuffer>
#include <QByteArray>
#include <QSignalSpy>
#include <QTime>
#include <QTimer>
#include <QtTest>
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

public:
    GCodeSenderTest();

private:
    void sendAcks(QTimer& timer, TestSerialPort* serialPort, int numAcks);

private Q_SLOTS:
    // TODO-TOMMY Tests here are too complex and fragile, we should perhaps refactor this class to
    // simplify its implementation (among other things, try to remove sleeps)
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
    void ignoreUnexpectedFirmwareResponses();
    void doNotSendMoreThan128BytesWithoutAReply();
    void doNotSendLineIfItWouldCauseMoreThan128BytesToBeSent();
    void doNotSendWireOffIfItWouldCauseMoreThan128BytesToBeSent();
    void doNotExpectAcksForWireControllerRealTimeCommands();
    void doNotWaitForAcksIfStreamInterrupted();
    void sendHardResetWhenStreamInterruptedByUser();
    void sendHardResetWhenStreamErrorOccurs();
    void sendHardResetWhenMachineRepliesWithError();
    void sendStreamingEndSignalAtStartIfErrorOccursBeforeStart();
    void endStreamingWithErrorIfInUnexpectedStatus();
    void waitSomeTimeAtTheEndBeforeCheckingMachineIsIdle();
    void waitSomeTimeAtStartAfterHardReset();
};

GCodeSenderTest::GCodeSenderTest()
{
}

void GCodeSenderTest::sendAcks(QTimer& timer, TestSerialPort* serialPort, int numAcks)
{
    int ackCount = -1; // -1 to take into account the processing of events right after the hard reset
    connect(&timer, &QTimer::timeout, [serialPort, &ackCount, numAcks](){
        if (ackCount >= 0 && ackCount < numAcks) {
            serialPort->simulateReceivedData("ok\r\n");
        } else if (ackCount == numAcks) {
            serialPort->simulateReceivedData("<Idle|MPos:0.000,0.000,0.000|FS:0,0|WCO:0.000,0.000,0.000>\r\n\r\n");
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
    MachineStatusMonitor statusMonitor(100000, communicator.get());

    auto bufferUptr = std::make_unique<TestBuffer>();
    bufferUptr->setParent(this);
    bufferUptr->buffer() = "G01 X100\n";
    TestBuffer* const buffer = bufferUptr.get();
    GCodeSender fileSender(1, 1, communicator.get(), &wireController, &statusMonitor, std::move(bufferUptr));

    QCOMPARE(buffer->parent(), nullptr);
}

void GCodeSenderTest::sendSignalWhenStreamingStartsAndEnds()
{
    auto communicatorAndPort = createCommunicator();
    auto communicator = std::move(communicatorAndPort.first);
    auto serialPort = communicatorAndPort.second;
    WireController wireController(communicator.get());
    // Very high polling delay to avoid having to check ? is sent, se simply send status when needed
    MachineStatusMonitor statusMonitor(100000, communicator.get());

    auto buffer = std::make_unique<TestBuffer>();
    buffer->buffer() = "G01 X100\n";
    GCodeSender fileSender(1, 1, communicator.get(), &wireController, &statusMonitor, std::move(buffer));

    QSignalSpy dataSentSpy(communicator.get(), &MachineCommunication::dataSent);
    QSignalSpy startSpy(&fileSender, &GCodeSender::streamingStarted);
    QSignalSpy endSpy(&fileSender, &GCodeSender::streamingEnded);

    // This is used to schedule a function to be executed when the QT event loop is executed
    // (interval is 0).
    QTimer timer;
    int ackCount = -1;
    QTime chrono;
    bool machineStatusSent = false;
    connect(&timer, &QTimer::timeout, [serialPort, &endSpy, &ackCount, &chrono, &machineStatusSent](){
        QCOMPARE(endSpy.count(), 0);
        if (ackCount == -1) {
            // This is sent by Grbl upon reset
            serialPort->simulateReceivedData("Grbl 1.1f ['$' for help]\r\n");
        } else if (ackCount < 3) {
            serialPort->simulateReceivedData("ok\r\n");
            chrono.start();
        } else if (!machineStatusSent) {
            // wait some time before signalling the machine has gone idle
            if (chrono.elapsed() > 1000) {
                serialPort->simulateReceivedData("<Idle|MPos:0.000,0.000,0.000|FS:0,0|WCO:0.000,0.000,0.000>\r\n");
                machineStatusSent = true;
            }
        }
        ++ackCount;
    });
    timer.start();

    fileSender.streamData();

    QVERIFY(machineStatusSent);
    QCOMPARE(dataSentSpy.count(), 6); // First 3 are ?, M5 and S30 sent by statuMonitor and wireController on hardReset
    QCOMPARE(dataSentSpy.at(3).at(0).toByteArray(), "M3\n");
    QCOMPARE(dataSentSpy.at(4).at(0).toByteArray(), "G01 X100\n");
    QCOMPARE(dataSentSpy.at(5).at(0).toByteArray(), "M5\n");
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
    MachineStatusMonitor statusMonitor(100000, communicator.get());

    auto bufferUptr = std::make_unique<TestBuffer>();
    bufferUptr->buffer() = "G01 X100\n";
    TestBuffer* const buffer = bufferUptr.get();
    GCodeSender fileSender(1, 1, communicator.get(), &wireController, &statusMonitor, std::move(bufferUptr));

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
    MachineStatusMonitor statusMonitor(100000, communicator.get());

    auto bufferUptr = std::make_unique<TestBuffer>();
    bufferUptr->buffer() = "G01 X100\n";
    TestBuffer* const buffer = bufferUptr.get();
    GCodeSender fileSender(1, 1, communicator.get(), &wireController, &statusMonitor, std::move(bufferUptr));

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
    MachineStatusMonitor statusMonitor(100000, communicator.get());

    auto bufferUptr = std::make_unique<TestBuffer>();
    bufferUptr->setOpenError(true);
    bufferUptr->buffer() = "G01 X100\n";
    TestBuffer* const buffer = bufferUptr.get();
    GCodeSender fileSender(1, 1, communicator.get(), &wireController, &statusMonitor, std::move(bufferUptr));

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
    MachineStatusMonitor statusMonitor(100000, communicator.get());

    auto bufferUptr = std::make_unique<TestBuffer>();
    bufferUptr->setReadError(true);
    bufferUptr->buffer() = "G01 X100\n";
    TestBuffer* const buffer = bufferUptr.get();
    GCodeSender fileSender(1, 1, communicator.get(), &wireController, &statusMonitor, std::move(bufferUptr));

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
    MachineStatusMonitor statusMonitor(100000, communicator.get());

    QSignalSpy spy(communicator.get(), &MachineCommunication::dataSent);

    auto buffer = std::make_unique<TestBuffer>();
    buffer->buffer() = "G01 X100";
    GCodeSender fileSender(1, 1, communicator.get(), &wireController, &statusMonitor, std::move(buffer));

    // This is used to schedule a function to be executed when the QT event loop is executed
    // (interval is 0).
    QTimer timer;
    sendAcks(timer, serialPort, 3);

    fileSender.streamData();

    QCOMPARE(spy.count(), 6); // First 3 are ?, M5 and S30 sent by statusMonitor and wireController on hardReset
    auto data = spy.at(4).at(0).toByteArray();
    QCOMPARE(data, "G01 X100\n");
}

void GCodeSenderTest::sendMultipleLines()
{
    auto communicatorAndPort = createCommunicator();
    auto communicator = std::move(communicatorAndPort.first);
    auto serialPort = communicatorAndPort.second;
    WireController wireController(communicator.get());
    MachineStatusMonitor statusMonitor(100000, communicator.get());

    QSignalSpy spy(communicator.get(), &MachineCommunication::dataSent);

    auto buffer = std::make_unique<TestBuffer>();
    buffer->buffer() = "G01 X100\nG01 Y1\nG00 Z9";
    GCodeSender fileSender(1, 1, communicator.get(), &wireController, &statusMonitor, std::move(buffer));

    QSignalSpy endSpy(&fileSender, &GCodeSender::streamingEnded);

    // This is used to schedule a function to be executed when the QT event loop is executed
    // (interval is 0).
    QTimer timer;
    sendAcks(timer, serialPort, 5);

    fileSender.streamData();

    QCOMPARE(spy.count(), 8); // 3 + 5 (1 ? plus 2 sent by wireController on hardReset plus wire on and off)
    QCOMPARE(spy.at(4).at(0).toByteArray(), "G01 X100\n");
    QCOMPARE(spy.at(5).at(0).toByteArray(), "G01 Y1\n");
    QCOMPARE(spy.at(6).at(0).toByteArray(), "G00 Z9\n");
    QCOMPARE(endSpy.count(), 1);
    auto reason = endSpy.at(0).at(0).value<GCodeSender::StreamEndReason>();
    QCOMPARE(reason, GCodeSender::StreamEndReason::Completed);
}

void GCodeSenderTest::sendStreamingEndSignalWithPortClosedReasonIfPortClosed()
{
    auto communicator = createCommunicator().first;
    WireController wireController(communicator.get());
    MachineStatusMonitor statusMonitor(100000, communicator.get());

    // This is used to schedule a function to be executed when the QT event loop is executed
    // (interval is 0)
    QTimer timer;
    connect(&timer, &QTimer::timeout, [communicator = communicator.get()](){ communicator->closePort(); });
    timer.start();

    auto bufferUptr = std::make_unique<TestBuffer>();
    bufferUptr->buffer() = "G01 X100\nG01 Y1\nG00 Z9";
    TestBuffer* const buffer = bufferUptr.get();
    GCodeSender fileSender(1, 1, communicator.get(), &wireController, &statusMonitor, std::move(bufferUptr));

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
    MachineStatusMonitor statusMonitor(100000, communicator.get());

    auto bufferUptr = std::make_unique<TestBuffer>();
    bufferUptr->buffer() = "G01 X100\nG01 Y1\nG00 Z9";
    TestBuffer* const buffer = bufferUptr.get();
    GCodeSender fileSender(1, 1, communicator.get(), &wireController, &statusMonitor, std::move(bufferUptr));

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
    MachineStatusMonitor statusMonitor(100000, communicator.get());

    auto bufferUptr = std::make_unique<TestBuffer>();
    bufferUptr->buffer() = "G01 X100\nG01 Y1\nG00 Z9";
    TestBuffer* const buffer = bufferUptr.get();
    GCodeSender fileSender(1, 1, communicator.get(), &wireController, &statusMonitor, std::move(bufferUptr));

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

void GCodeSenderTest::ignoreUnexpectedFirmwareResponses()
{
    auto communicatorAndPort = createCommunicator();
    auto communicator = std::move(communicatorAndPort.first);
    auto serialPort = communicatorAndPort.second;
    WireController wireController(communicator.get());
    MachineStatusMonitor statusMonitor(100000, communicator.get());

    auto buffer = std::make_unique<TestBuffer>();
    buffer->buffer() = "G01 X100\nG01 Y1\nG00 Z9";
    GCodeSender fileSender(1, 1, communicator.get(), &wireController, &statusMonitor, std::move(buffer));

    // This is used to schedule a function to be executed when the QT event loop is executed
    // (interval is 0)
    QTimer timer;
    int count = 0;
    int okSent = -1; // -1 to take into account the message processing ritgh after the hard reset
    bool idleSent = false;
    connect(&timer, &QTimer::timeout, [serialPort, &count, &okSent, &idleSent](){
        if (count == 1) {
            serialPort->simulateReceivedData("unexpected\r\n");
        } else if (okSent < 5) {
            if (okSent >= 0) {
                serialPort->simulateReceivedData("ok\r\n");
            }
            ++okSent;
        } else if (!idleSent) {
            serialPort->simulateReceivedData("<Idle|MPos:0.000,0.000,0.000|FS:0,0|WCO:0.000,0.000,0.000>\r\n");
            idleSent = true;
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
    GrblBufferLimitTestHelper(int limitByte, MachineCommunication* communicator, TestSerialPort* serialPort, GCodeSender* sender, bool interruptAtLimitByte = false)
        : m_limitByte(limitByte)
        , m_interruptAtLimitByte(interruptAtLimitByte)
        , m_serialPort(serialPort)
        , m_sender(sender)
        , m_bytesSent(0)
        , m_pauseCyclesAtLimitByte(0)
        , m_expectStartAgainSendingData(false)
        , m_startedAgainSendingData(true)
        , m_acksToSend(-2) // Compensates for M5 and S30 sent by wireController on hardReset
        , m_machineGoesIdle(false)
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
        // The immediate command does not count for acks
        if (data == "?") {
            return;
        }

        if (m_acksToSend >= 0) {
            m_bytesSent += data.size();
        }
        ++m_acksToSend;

        // Stream always starts with M3 and ends with M5, when we get it, we can set status to Idle.
        // The check on M3 is needed because another M5 is sent by wireController after hard reset
        if (data == "M3\n") {
            m_machineGoesIdle = false;
        } else if (data == "M5\n") {
            m_machineGoesIdle = true;
        }
    }

    void timeout()
    {
        if (m_bytesSent < m_limitByte) {
            return;
        } else if (m_bytesSent == m_limitByte && m_pauseCyclesAtLimitByte < 3) {
            if (m_interruptAtLimitByte) {
                if (m_pauseCyclesAtLimitByte == 0) {
                    m_sender->interruptStreaming();
                    // We only set m_pauseCuclesAtLimitByte to 1 so that we interrupt once but never
                    // send any ack
                    m_pauseCyclesAtLimitByte = 1;
                }
            } else {
                ++m_pauseCyclesAtLimitByte;
            }
        } else {
            if (m_expectStartAgainSendingData) {
                // This becomes true only if GCodeSender has sent more data right after the first ack
                m_startedAgainSendingData = true;
                m_expectStartAgainSendingData = false;
            }
            if (m_acksToSend > 0) {
                m_serialPort->simulateReceivedData("ok\r\n");
                --m_acksToSend;
            } else if (m_machineGoesIdle) {
                m_serialPort->simulateReceivedData("<Idle|MPos:0.000,0.000,0.000|FS:0,0|WCO:0.000,0.000,0.000>\r\n");
            }
            m_expectStartAgainSendingData = true;
        }
    }

private:
    QTimer m_timer;
    // At what byte sending should stop. This can be less than 128 if sending another line would
    // cause the 128 limit to be crossed
    const int m_limitByte;
    const bool m_interruptAtLimitByte;
    TestSerialPort* const m_serialPort;
    GCodeSender* const m_sender;
    int m_bytesSent;
    int m_pauseCyclesAtLimitByte;
    bool m_expectStartAgainSendingData;
    bool m_startedAgainSendingData;
    int m_acksToSend;
    bool m_machineGoesIdle;
};

void GCodeSenderTest::doNotSendMoreThan128BytesWithoutAReply()
{
    auto communicatorAndPort = createCommunicator();
    auto communicator = std::move(communicatorAndPort.first);
    auto serialPort = communicatorAndPort.second;
    WireController wireController(communicator.get());
    MachineStatusMonitor statusMonitor(100000, communicator.get());

    auto buffer = std::make_unique<TestBuffer>();
    // 3 bytes (wire on) + 13 bytes + 14 times 8 bytes = 128 bytes. Then 3 more
    // instructions - total 19 times 8 bytes
    buffer->buffer() += "G01 X123 Y45\n";
    for (auto i = 0; i < 17; ++i) {
        buffer->buffer() += "G01 X10\n";
    }
    GCodeSender fileSender(1, 1, communicator.get(), &wireController, &statusMonitor, std::move(buffer));

    GrblBufferLimitTestHelper testHelper(128, communicator.get(), serialPort, &fileSender);

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
    MachineStatusMonitor statusMonitor(100000, communicator.get());

    auto buffer = std::make_unique<TestBuffer>();
    // 3 bytes (wire on) + 15 times 8 bytes = 123 bytes
    for (auto i = 0; i < 15; ++i) {
        buffer->buffer() += "G01 X10\n";
    }
    // Now 4 bytes -> 127 bytes
    buffer->buffer() += "G01\n";
    // Next line would cause the 128 bytes limit to be crossed, should wait before sending
    buffer->buffer() += "G01 X10\n";
    GCodeSender fileSender(1, 1, communicator.get(), &wireController, &statusMonitor, std::move(buffer));

    GrblBufferLimitTestHelper testHelper(127, communicator.get(), serialPort, &fileSender);

    fileSender.streamData();

    QCOMPARE(testHelper.pauseCyclesAtLimitByte(), 3);
    QVERIFY(testHelper.startedAgainSendingData());
}

void GCodeSenderTest::doNotSendWireOffIfItWouldCauseMoreThan128BytesToBeSent()
{
    auto communicatorAndPort = createCommunicator();
    auto communicator = std::move(communicatorAndPort.first);
    auto serialPort = communicatorAndPort.second;
    WireController wireController(communicator.get());
    MachineStatusMonitor statusMonitor(100000, communicator.get());

    auto buffer = std::make_unique<TestBuffer>();
    // 3 bytes (wire on) + 14 times 8 bytes = 115 bytes
    for (auto i = 0; i < 14; ++i) {
        buffer->buffer() += "G01 X10\n";
    }
    // Now 12 bytes -> 127 bytes
    buffer->buffer() += "G01 X22 Y31\n";
    GCodeSender fileSender(1, 1, communicator.get(), &wireController, &statusMonitor, std::move(buffer));

    GrblBufferLimitTestHelper testHelper(127, communicator.get(), serialPort, &fileSender);

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
    MachineStatusMonitor statusMonitor(100000, communicator.get());

    auto buffer = std::make_unique<TestBuffer>();
    buffer->buffer() = "G01 Y1\nG00 Z9\n";
    GCodeSender fileSender(1, 1, communicator.get(), &wireController, &statusMonitor, std::move(buffer));

    QSignalSpy endSpy(&fileSender, &GCodeSender::streamingEnded);

    QTimer timer;
    int count = -1; // this is to take into account the message processing right after the hard reset
    connect(&timer, &QTimer::timeout, [&count, serialPort, &wireController](){
        if (count >= 0 && count < 4) {
            serialPort->simulateReceivedData("ok\r\n");
        } else if (count == 4) {
            serialPort->simulateReceivedData("<Idle|MPos:0.000,0.000,0.000|FS:0,0|WCO:0.000,0.000,0.000>\r\n");
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

void GCodeSenderTest::doNotWaitForAcksIfStreamInterrupted()
{
    auto communicatorAndPort = createCommunicator();
    auto communicator = std::move(communicatorAndPort.first);
    auto serialPort = communicatorAndPort.second;
    WireController wireController(communicator.get());
    MachineStatusMonitor statusMonitor(100000, communicator.get());

    auto buffer = std::make_unique<TestBuffer>();
    // 3 bytes (wire on) + 14 times 8 bytes = 115 bytes
    for (auto i = 0; i < 14; ++i) {
        buffer->buffer() += "G01 X10\n";
    }
    // Now 12 bytes -> 127 bytes
    buffer->buffer() += "G01 X11 Y77\n";
    // Next line would cause the 128 bytes limit to be crossed, should wait before sending
    buffer->buffer() += "G01 X10\n";
    GCodeSender fileSender(1, 1, communicator.get(), &wireController, &statusMonitor, std::move(buffer));

    QSignalSpy endSpy(&fileSender, &GCodeSender::streamingEnded);

    GrblBufferLimitTestHelper testHelper(127, communicator.get(), serialPort, &fileSender, true);

    fileSender.streamData();

    QCOMPARE(endSpy.count(), 1);
    auto reason = endSpy.at(0).at(0).value<GCodeSender::StreamEndReason>();
    QCOMPARE(reason, GCodeSender::StreamEndReason::UserInterrupted);
}

void GCodeSenderTest::sendHardResetWhenStreamInterruptedByUser()
{
    auto communicator = createCommunicator().first;
    WireController wireController(communicator.get());
    MachineStatusMonitor statusMonitor(100000, communicator.get());

    QSignalSpy machineInitializedSpy(communicator.get(), &MachineCommunication::machineInitialized);

    auto buffer = std::make_unique<TestBuffer>();
    buffer->buffer() = "G01 X100\nG01 Y1\nG00 Z9";
    GCodeSender fileSender(1, 1, communicator.get(), &wireController, &statusMonitor, std::move(buffer));

    // This is used to schedule a function to be executed when the QT event loop is executed
    // (interval is 0)
    QTimer timer;
    connect(&timer, &QTimer::timeout, [&fileSender](){ fileSender.interruptStreaming(); });
    timer.start();

    QSignalSpy endSpy(&fileSender, &GCodeSender::streamingEnded);

    fileSender.streamData();

    QCOMPARE(endSpy.count(), 1);
    auto reason = endSpy.at(0).at(0).value<GCodeSender::StreamEndReason>();
    QCOMPARE(reason, GCodeSender::StreamEndReason::UserInterrupted);
    QCOMPARE(machineInitializedSpy.count(), 2); // 1 for hard reset at the begin of streamData
}

void GCodeSenderTest::sendHardResetWhenStreamErrorOccurs()
{
    auto communicator = createCommunicator().first;
    WireController wireController(communicator.get());
    MachineStatusMonitor statusMonitor(100000, communicator.get());

    QSignalSpy machineInitializedSpy(communicator.get(), &MachineCommunication::machineInitialized);

    auto buffer = std::make_unique<TestBuffer>();
    buffer->setReadError(true);
    buffer->buffer() = "G01 X100\n";
    GCodeSender fileSender(1, 1, communicator.get(), &wireController, &statusMonitor, std::move(buffer));

    QSignalSpy endSpy(&fileSender, &GCodeSender::streamingEnded);

    fileSender.streamData();

    QCOMPARE(endSpy.count(), 1);
    auto reason = endSpy.at(0).at(0).value<GCodeSender::StreamEndReason>();
    QCOMPARE(reason, GCodeSender::StreamEndReason::StreamError);
    QCOMPARE(machineInitializedSpy.count(), 2); // 1 for hard reset at the begin of streamData
}

void GCodeSenderTest::sendHardResetWhenMachineRepliesWithError()
{
    auto communicatorAndPort = createCommunicator();
    auto communicator = std::move(communicatorAndPort.first);
    auto serialPort = communicatorAndPort.second;
    WireController wireController(communicator.get());
    MachineStatusMonitor statusMonitor(100000, communicator.get());

    QSignalSpy machineInitializedSpy(communicator.get(), &MachineCommunication::machineInitialized);

    auto buffer = std::make_unique<TestBuffer>();
    buffer->buffer() = "G01 X100\nG01 Y1\nG00 Z9";
    GCodeSender fileSender(1, 1, communicator.get(), &wireController, &statusMonitor, std::move(buffer));

    // This is used to schedule a function to be executed when the QT event loop is executed
    // (interval is 0)
    QTimer timer;
    connect(&timer, &QTimer::timeout, [serialPort](){ serialPort->simulateReceivedData("error:10\r\n"); });
    timer.start();

    QSignalSpy endSpy(&fileSender, &GCodeSender::streamingEnded);

    fileSender.streamData();

    QCOMPARE(endSpy.count(), 1);
    auto reason = endSpy.at(0).at(0).value<GCodeSender::StreamEndReason>();
    QCOMPARE(reason, GCodeSender::StreamEndReason::MachineErrorReply);
    QCOMPARE(machineInitializedSpy.count(), 2); // 1 for hard reset at the begin of streamData
}

void GCodeSenderTest::sendStreamingEndSignalAtStartIfErrorOccursBeforeStart()
{
    auto communicatorAndPort = createCommunicator();
    auto communicator = std::move(communicatorAndPort.first);
    auto serialPort = communicatorAndPort.second;
    WireController wireController(communicator.get());
    MachineStatusMonitor statusMonitor(100000, communicator.get());

    auto bufferUptr = std::make_unique<TestBuffer>();
    bufferUptr->buffer() = "G01 X100\nG01 Y1\nG00 Z9";
    TestBuffer* const buffer = bufferUptr.get();
    GCodeSender fileSender(1, 1, communicator.get(), &wireController, &statusMonitor, std::move(bufferUptr));

    QSignalSpy deviceDeleted(buffer, &QIODevice::destroyed);
    QSignalSpy endSpy(&fileSender, &GCodeSender::streamingEnded);

    // Error before start
    serialPort->simulateReceivedData("error:10\r\n");

    fileSender.streamData();

    QCOMPARE(endSpy.count(), 1);
    auto reason = endSpy.at(0).at(0).value<GCodeSender::StreamEndReason>();
    auto description = endSpy.at(0).at(1).toString();
    QCOMPARE(reason, GCodeSender::StreamEndReason::MachineErrorReply);
    QCOMPARE(description, tr("Firmware replied with error: ") + "10");
    QCOMPARE(deviceDeleted.count(), 1);
}

void GCodeSenderTest::endStreamingWithErrorIfInUnexpectedStatus()
{
    auto communicatorAndPort = createCommunicator();
    auto communicator = std::move(communicatorAndPort.first);
    auto serialPort = communicatorAndPort.second;
    WireController wireController(communicator.get());
    MachineStatusMonitor statusMonitor(100000, communicator.get());

    auto buffer = std::make_unique<TestBuffer>();
    buffer->buffer() = "G01 Y1\nG00 Z9\n";
    GCodeSender fileSender(1, 1, communicator.get(), &wireController, &statusMonitor, std::move(buffer));

    QSignalSpy endSpy(&fileSender, &GCodeSender::streamingEnded);

    QTimer timer;
    // We send no acks, just switch status
    int count;
    bool inUnexpectedState = false;
    connect(&timer, &QTimer::timeout, [&count, &inUnexpectedState, serialPort, &wireController](){
        switch (count) {
            case 0:
                serialPort->simulateReceivedData("<Idle|MPos:0.000,0.000,0.000|FS:0,0|WCO:0.000,0.000,0.000>\r\n");
                break;
            case 1:
                serialPort->simulateReceivedData("<Run|MPos:0.000,0.000,0.000|FS:0,0|WCO:0.000,0.000,0.000>\r\n");
                break;
            case 2:
                serialPort->simulateReceivedData("<Hold|MPos:0.000,0.000,0.000|FS:0,0|WCO:0.000,0.000,0.000>\r\n");
                break;
            case 3:
                // Not a valid state, mapped to MachineState::Unknown
                serialPort->simulateReceivedData("<Unknown|MPos:0.000,0.000,0.000|FS:0,0|WCO:0.000,0.000,0.000>\r\n");
                break;
            case 4:
                serialPort->simulateReceivedData("<Alarm|MPos:0.000,0.000,0.000|FS:0,0|WCO:0.000,0.000,0.000>\r\n");
                inUnexpectedState = true;
                break;
            default:
                break;
        }
        ++count;
    });
    timer.start();

    fileSender.streamData();

    QVERIFY(inUnexpectedState);
    QCOMPARE(endSpy.count(), 1);
    auto reason = endSpy.at(0).at(0).value<GCodeSender::StreamEndReason>();
    QCOMPARE(reason, GCodeSender::StreamEndReason::MachineErrorReply);
}

void GCodeSenderTest::waitSomeTimeAtTheEndBeforeCheckingMachineIsIdle()
{
    // This is needed because with short g-codes we might send everything anche check the machine is
    // idle before the machine switches to Run state (or before we poll the machine state and find
    // out is is running). We also cannot be sure we will be notified that the machine is running,
    // because e.g. a g-code that is simply "M3" the machine will never be in Run state (so we
    // cannot wait for the machine to go Idle after being in Run). So solve this issue we simply
    // wait few seconds before starting to check that the machine is idle at the end of streaming

    auto communicatorAndPort = createCommunicator();
    auto communicator = std::move(communicatorAndPort.first);
    auto serialPort = communicatorAndPort.second;
    WireController wireController(communicator.get());
    // Very high polling delay to avoid having to check ? is sent, se simply send status when needed
    MachineStatusMonitor statusMonitor(100000, communicator.get());

    auto buffer = std::make_unique<TestBuffer>();
    buffer->buffer() = "G01 X100\n";
    GCodeSender fileSender(1, 1000, communicator.get(), &wireController, &statusMonitor, std::move(buffer));

    // This is used to schedule a function to be executed when the QT event loop is executed
    // (interval is 0).
    QTimer timer;
    int count = -1; // This is to take into account the message processing right after the hard reset
    QTime chrono;
    bool enoughTimeWaited = false;
    connect(&timer, &QTimer::timeout, [serialPort, &count, &chrono, &enoughTimeWaited](){
        switch (count) {
            case -1:
                break;
            case 0:
                chrono.start();
                break;
            case 1:
                serialPort->simulateReceivedData("ok\r\n");
                serialPort->simulateReceivedData("ok\r\n");
                serialPort->simulateReceivedData("ok\r\n");
                serialPort->simulateReceivedData("<Idle|MPos:0.000,0.000,0.000|FS:0,0|WCO:0.000,0.000,0.000>\r\n");
                break;
            default:
                if (chrono.elapsed() > 900) {
                    enoughTimeWaited = true;
                }
                break;
        }
        ++count;
    });
    timer.start();

    fileSender.streamData();

    QVERIFY(enoughTimeWaited);
}

void GCodeSenderTest::waitSomeTimeAtStartAfterHardReset()
{
    // This is needed to make sure that "ok" for temperature setting commands after a hard reset are
    // received before we start sending data and are then not computed as replies of streamed g-code
    // commands.

    auto communicatorAndPort = createCommunicator();
    auto communicator = std::move(communicatorAndPort.first);
    auto serialPort = communicatorAndPort.second;
    WireController wireController(communicator.get());
    // Very high polling delay to avoid having to check ? is sent, se simply send status when needed
    MachineStatusMonitor statusMonitor(100000, communicator.get());

    auto buffer = std::make_unique<TestBuffer>();
    buffer->buffer() = "G01 X100\n";
    GCodeSender fileSender(1000, 1, communicator.get(), &wireController, &statusMonitor, std::move(buffer));

    // This is used to schedule a function to be executed when the QT event loop is executed
    // (interval is 0).
    QTimer timer;
    int count = -1; // This is to take into account the message processing right after the hard reset
    QTime chrono;
    bool enoughTimeWaited = false;
    connect(&timer, &QTimer::timeout, [serialPort, &count, &chrono, &enoughTimeWaited](){
        switch (count) {
            case -1:
            case 0:
                break;
            case 1:
                serialPort->simulateReceivedData("ok\r\n");
                serialPort->simulateReceivedData("ok\r\n");
                serialPort->simulateReceivedData("ok\r\n");
                serialPort->simulateReceivedData("<Idle|MPos:0.000,0.000,0.000|FS:0,0|WCO:0.000,0.000,0.000>\r\n");
                break;
            default:
                if (chrono.elapsed() > 900) {
                    enoughTimeWaited = true;
                }
                break;
        }
        ++count;
    });
    timer.start();

    chrono.start();
    fileSender.streamData();

    QVERIFY(enoughTimeWaited);
}

QTEST_GUILESS_MAIN(GCodeSenderTest)

#include "gcodesender_test.moc"

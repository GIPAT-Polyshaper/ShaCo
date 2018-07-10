#include "gcodesender.h"
#include <numeric>
#include <QCoreApplication>
#include <QRegularExpression>
#include <QString>
#include <QThread>

namespace {
    constexpr int grblBufferSize = 128;
    const QRegularExpression okRegExpr("^ok$", QRegularExpression::OptimizeOnFirstUsageOption);
    const QRegularExpression errorRegExpr("^error:([0-9]+)$", QRegularExpression::OptimizeOnFirstUsageOption);

    bool registerStreamEndReason()
    {
        static bool registered = false;

        if (!registered) {
            qRegisterMetaType<GCodeSender::StreamEndReason>();

            registered = true;
        }

        return registered;
    }
}

const bool GCodeSender::streamEndReasonRegistered = registerStreamEndReason();

GCodeSender::GCodeSender(MachineCommunication* communicator, WireController* wireController, std::unique_ptr<QIODevice>&& gcodeDevice)
    : QObject(nullptr)
    , m_communicator(communicator)
    , m_wireController(wireController)
    , m_device(std::move(gcodeDevice))
    , m_streamEndReason(StreamEndReason::Completed)
{
    m_device->setParent(nullptr);

    connect(m_communicator, &MachineCommunication::portClosedWithError, this, &GCodeSender::portClosedWithError);
    connect(m_communicator, &MachineCommunication::portClosed, this, &GCodeSender::portClosed);
    connect(m_communicator, &MachineCommunication::messageReceived, this, &GCodeSender::messageReceived);
}

void GCodeSender::streamData()
{
    emit streamingStarted();

    if (!m_device) {
        emitStreamingEnded();
        return;
    } else if (!m_device->open(QIODevice::ReadOnly)) {
        closeStream(StreamEndReason::StreamError, tr("Input device could not be opened"));
        emitStreamingEnded();
        return;
    }

    // This works because all connection are synchronous (i.e. function calls), we can send data
    // only after port has been really opened
    m_communicator->hardReset();

    // Switching wire on at start
    m_wireController->switchWireOn();
    m_sentBytes.enqueue(m_wireController->switchWireOnCommandLength());

    while (m_device && !m_device->atEnd()) {
        auto line = m_device->readLine(grblBufferSize - 1);

        // NOTE: Inability to read a line is considered an error (we check above that the device is
        // not atEnd)
        if (line.isEmpty()) {
            closeStream(StreamEndReason::StreamError, tr("Could not read GCode line from input device"));
            break;
        }

        if (!line.endsWith('\n')) {
            line += '\n';
        }

        waitWhileBufferFull(line.size());
        m_communicator->writeData(line);
        m_sentBytes.enqueue(line.size());

        // Process QT events
        QCoreApplication::processEvents();
    }

    // Switching wire off at the end (if device is closed we either lost machine communication or
    // machine was hard reset). Grbl does not send ok until the wire off command is really executed,
    // so we don't exit from the cycle below until work has really ended
    if (m_device) {
        waitWhileBufferFull(m_wireController->switchWireOffCommandLength());
        m_wireController->switchWireOff();
        m_sentBytes.enqueue(m_wireController->switchWireOffCommandLength());
    }

    // Waiting remaining acks
    while (m_device && !m_sentBytes.isEmpty()) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);

        // Small delay, to avoid consuming too much cpu
        QThread::usleep(500);
    }

    if (m_device) {
        closeStream(StreamEndReason::Completed, tr("Success"));
    }

    emitStreamingEnded();
}

void GCodeSender::interruptStreaming()
{
    closeStream(StreamEndReason::UserInterrupted, tr("Streaming interrupted by the user"));
}

void GCodeSender::portClosedWithError()
{
    closeStream(StreamEndReason::PortError, tr("Serial port closed with error"));
}

void GCodeSender::portClosed()
{
    closeStream(StreamEndReason::PortClosed, tr("Serial port closed"));
}

void GCodeSender::messageReceived(QByteArray message)
{
    if (okRegExpr.match(message).hasMatch()) {
        if (!m_sentBytes.isEmpty()) {
            m_sentBytes.dequeue();
        }
    } else {
        auto match = errorRegExpr.match(message);
        if (match.hasMatch()) {
            closeStream(StreamEndReason::MachineErrorReply, tr("Firmware replied with error: ") + match.captured(1));
        }
    }
}

void GCodeSender::closeStream(GCodeSender::StreamEndReason reason, QString description)
{
    // For some StreamEndResons we also send a soft reset
    switch (reason) {
        case StreamEndReason::MachineErrorReply:
        case StreamEndReason::StreamError:
        case StreamEndReason::UserInterrupted:
            m_communicator->hardReset();
            break;
        default:
            break;
    }

    m_device.reset();
    m_streamEndReason = reason;
    m_streamEndDescription = description;
}

int GCodeSender::bytesSentSinceLastAck() const
{
    return std::accumulate(m_sentBytes.constBegin(), m_sentBytes.constEnd(), 0);
}

void GCodeSender::waitWhileBufferFull(int requiredSpace)
{
    // Processing QT events while waiting for firmware buffer to find space
    while (m_device && bytesSentSinceLastAck() + requiredSpace > grblBufferSize) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);

        // Small delay, to avoid consuming too much cpu
        QThread::usleep(500);
    }
}

void GCodeSender::emitStreamingEnded()
{
    emit streamingEnded(m_streamEndReason, m_streamEndDescription);
}

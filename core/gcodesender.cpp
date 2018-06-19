#include "gcodesender.h"
#include <numeric>
#include <QCoreApplication>
#include <QRegularExpression>
#include <QString>

namespace {
    constexpr int grblBufferSize = 128;
    const QRegularExpression okRegExpr("ok", QRegularExpression::OptimizeOnFirstUsageOption);
    const QRegularExpression errorRegExpr("error:([0-9]+)", QRegularExpression::OptimizeOnFirstUsageOption);

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
    , m_paused(false)
{
    m_device->setParent(nullptr);

    connect(m_communicator, &MachineCommunication::portClosedWithError, this, &GCodeSender::portClosedWithError);
    connect(m_communicator, &MachineCommunication::portClosed, this, &GCodeSender::portClosed);
    connect(m_communicator, &MachineCommunication::dataReceived, this, &GCodeSender::dataReceived);
}

bool GCodeSender::paused() const
{
    return m_paused;
}

void GCodeSender::streamData()
{
    emit streamingStarted();

    if (!m_device->open(QIODevice::ReadOnly)) {
        closeStream(StreamEndReason::StreamError, tr("Input device could not be opened"));
        return;
    }

    // Switching wire on at start
    m_wireController->switchWireOn();
    m_sentBytes.enqueue(m_wireController->switchWireOnCommandLength());

    while (m_device && !m_device->atEnd()) {
        auto line = m_device->readLine(grblBufferSize - 1);

        // NOTE: Inability to read a line is considered an error (we check above that the device is
        // not atEnd)
        if (line.isEmpty()) {
            closeStream(StreamEndReason::StreamError, tr("Could not read GCode line from input device"));
            return;
        }

        if (!line.endsWith('\n')) {
            line += '\n';
        }

        waitWhilePausedOrBufferFull(line.size());
        m_communicator->writeData(line);
        m_sentBytes.enqueue(line.size());

        // Process QT events
        QCoreApplication::processEvents();
    }

    // Switching wire off at the end
    waitWhilePausedOrBufferFull(m_wireController->switchWireOffCommandLength());
    m_wireController->switchWireOff();
    m_sentBytes.enqueue(m_wireController->switchWireOffCommandLength());

    // Waiting remaining acks
    while (m_device && !m_sentBytes.isEmpty()) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
    }

    if (m_device) {
        closeStream(StreamEndReason::Completed, tr("Success"));
    }
}

void GCodeSender::interruptStreaming()
{
    m_paused = false;
    closeStream(StreamEndReason::UserInterrupted, tr("Streaming interrupted by the user"));
}

void GCodeSender::pause()
{
    if (m_paused) {
        return;
    }

    m_paused = true;
    emit streamingPaused();
}

void GCodeSender::resume()
{
    if (!m_paused) {
        return;
    }

    m_paused = false;
    emit streamingResumed();
}

void GCodeSender::portClosedWithError()
{
    closeStream(StreamEndReason::PortError, tr("Serial port closed with error"));
}

void GCodeSender::portClosed()
{
    closeStream(StreamEndReason::PortClosed, tr("Serial port closed"));
}

void GCodeSender::dataReceived(QByteArray data)
{
    m_partialReply += data;

    auto endCommand = findEndCommandInPartialReply();
    while (endCommand != -1) {
        const auto& reply = m_partialReply.left(endCommand);
        m_partialReply = m_partialReply.mid(endCommand + 2);

        if (okRegExpr.match(reply).hasMatch()) {
            if (!m_sentBytes.isEmpty()) {
                m_sentBytes.dequeue();
            }
        } else {
            auto match = errorRegExpr.match(reply);
            if (match.hasMatch()) {
                closeStream(StreamEndReason::MachineErrorReply, tr("Firmware replied with error: ") + match.captured(1));
            }
        }

        endCommand = findEndCommandInPartialReply();
    }
}

void GCodeSender::closeStream(GCodeSender::StreamEndReason reason, QString description)
{
    m_device.reset();
    emit streamingEnded(reason, description);
}

int GCodeSender::findEndCommandInPartialReply() const
{
    return m_partialReply.indexOf("\r\n");
}

int GCodeSender::bytesSentSinceLastAck() const
{
    return std::accumulate(m_sentBytes.constBegin(), m_sentBytes.constEnd(), 0);
}

void GCodeSender::waitWhilePausedOrBufferFull(int requiredSpace)
{
    // Processing QT events while waiting for firmware buffer to find space or while in pause
    while (m_paused || bytesSentSinceLastAck() + requiredSpace > grblBufferSize) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
    }
}

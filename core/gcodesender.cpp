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

GCodeSender::GCodeSender(int hardResetDelay, int idleWaitInterval, MachineCommunication* communicator, WireController* wireController, MachineStatusMonitor *machineStatusMonitor, std::unique_ptr<QIODevice>&& gcodeDevice)
    : QObject(nullptr)
    , m_hardResetDelay(hardResetDelay)
    , m_idleWaitInterval(idleWaitInterval)
    , m_communicator(communicator)
    , m_wireController(wireController)
    , m_machineStatusMonitor(machineStatusMonitor)
    , m_device(std::move(gcodeDevice))
    , m_streamEndReason(StreamEndReason::Completed)
{
    m_device->setParent(nullptr);

    connect(m_communicator, &MachineCommunication::portClosedWithError, this, &GCodeSender::portClosedWithError);
    connect(m_communicator, &MachineCommunication::portClosed, this, &GCodeSender::portClosed);
    connect(m_communicator, &MachineCommunication::messageReceived, this, &GCodeSender::messageReceived);
    connect(m_machineStatusMonitor, &MachineStatusMonitor::stateChanged, this, &GCodeSender::stateChanged);
}

void GCodeSender::streamData()
{
    emit streamingStarted();

    if (!m_device) {
        emitStreamingEnded();
        return;
    } else if (!m_device->open(QIODevice::ReadOnly | QIODevice::Text)) {
        closeStream(StreamEndReason::StreamError, tr("Input device could not be opened"));
        emitStreamingEnded();
        return;
    }

    // This works because all connection are synchronous (i.e. function calls), we can send data
    // only after port has been really opened
    m_communicator->hardReset();

    // This is to make sure replies to commands sent by wireController after hard reset are received
    // before we start sending data, and are then not considered answers to streamed g-code
    QThread::msleep(m_hardResetDelay);
    QCoreApplication::processEvents();

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
    // machine was hard reset)
    if (m_device) {
        waitWhileBufferFull(m_wireController->switchWireOffCommandLength());
        m_wireController->switchWireOff();
        m_sentBytes.enqueue(m_wireController->switchWireOffCommandLength());
    }

    QThread::msleep(m_idleWaitInterval);

    // Waiting remaining acks and for machine to go Idle
    while (m_device && (m_machineStatusMonitor->state() != MachineState::Idle || !m_sentBytes.isEmpty())) {
        processEvents();
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

void GCodeSender::stateChanged(MachineState newState)
{
    if (newState != MachineState::Idle && newState != MachineState::Run &&
            newState != MachineState::Hold && newState != MachineState::Unknown) {
        closeStream(StreamEndReason::MachineErrorReply,
                    tr("Machine changed to unexpected state: ") + machineState2String(newState));
    }
}

void GCodeSender::closeStream(GCodeSender::StreamEndReason reason, QString description)
{
    // For some StreamEndResons we also send a hard reset
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
        processEvents();
    }
}

void GCodeSender::emitStreamingEnded()
{
    emit streamingEnded(m_streamEndReason, m_streamEndDescription);
}

void GCodeSender::processEvents()
{
    QCoreApplication::processEvents(QEventLoop::AllEvents, 100);

    // Small delay, to avoid consuming too much cpu
    QThread::usleep(500);
}

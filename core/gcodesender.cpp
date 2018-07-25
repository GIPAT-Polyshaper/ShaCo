#include "gcodesender.h"
#include <numeric>
#include <QCoreApplication>
#include <QRegularExpression>
#include <QString>
#include <QThread>
#include <QTime>

namespace {
    // This is the maximum allowed number commands to send in commandSender
    constexpr int maxQueuedToSendCommands = 10;
    // This is only to avoid exhausting memory for large wrong files
    constexpr int maxBytesInLine = 1000;

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

GCodeSender::GCodeSender(MachineCommunication* communicator, CommandSender* commandSender, WireController* wireController, MachineStatusMonitor *machineStatusMonitor, std::unique_ptr<QIODevice>&& gcodeDevice)
    : m_communicator(communicator)
    , m_commandSender(commandSender)
    , m_wireController(wireController)
    , m_machineStatusMonitor(machineStatusMonitor)
    , m_device(std::move(gcodeDevice))
    , m_running(false)
    , m_expectedAcks(0)
    , m_startedSendingCommands(false)
{
    m_device->setParent(nullptr);

    connect(m_machineStatusMonitor, &MachineStatusMonitor::stateChanged, this, &GCodeSender::stateChanged);
}

void GCodeSender::streamData()
{
    if (!m_device || m_device->isOpen()) {
        return;
    }

    emit streamingStarted();

    if (!m_device->open(QIODevice::ReadOnly | QIODevice::Text)) {
        emitStreamingEndedAndReset(StreamEndReason::StreamError, tr("Input device could not be opened"));
        return;
    }

    if (m_machineStatusMonitor->state() == MachineState::Idle) {
        startSendingCommands();
    }
}

void GCodeSender::interruptStreaming()
{
    if (m_device) {
        emitStreamingEndedAndReset(StreamEndReason::UserInterrupted, tr("User interrupted streaming"));
    }
}

void GCodeSender::stateChanged(MachineState newState)
{
    if (newState == MachineState::Idle) {
        if (!m_running && !m_startedSendingCommands) {
            startSendingCommands();
        } else if (canSuccessfullyFinishStreaming()) {
            finishStreaming();
        }
    } else if (newState == MachineState::Run) {
        m_running = true;
    } else if (newState != MachineState::Hold && newState != MachineState::Unknown) {
        emitStreamingEndedAndReset(StreamEndReason::MachineError,
                                   tr("Machine changed to unexpected state: ") + machineState2String(newState));
    }
}

void GCodeSender::commandSent(CommandCorrelationId)
{
    readAndSendOneCommand();

    while (!m_device->atEnd() && m_commandSender->pendingCommands() <= maxQueuedToSendCommands) {
        readAndSendOneCommand();
    }
}

void GCodeSender::okReply(CommandCorrelationId)
{
    --m_expectedAcks;

    if (canSuccessfullyFinishStreaming()) {
        finishStreaming();
    }
}

void GCodeSender::errorReply(CommandCorrelationId, int errorCode)
{
    emitStreamingEndedAndReset(StreamEndReason::MachineError,
                               tr("Firmware replied with error:") + QString::number(errorCode));
}

void GCodeSender::replyLost(CommandCorrelationId, bool)
{
    // This is needed to avoid continuous resets and calls to this function (test hangs)
    if (m_device) {
        emitStreamingEndedAndReset(StreamEndReason::PortError, tr("Failed to get replies for some commands"));
    }
}

void GCodeSender::readAndSendOneCommand()
{
    if (m_device && !m_device->atEnd()) {
        auto line = m_device->readLine(maxBytesInLine);
        if (line.isEmpty()) {
            emitStreamingEndedAndReset(StreamEndReason::StreamError, tr("Could not read GCode line from input device"));
        } else if (!m_commandSender->sendCommand(line, 0, this)) {
            emitStreamingEndedAndReset(StreamEndReason::StreamError, tr("Invalid command in GCode stream"));
        } else {
            ++m_expectedAcks;
        }
    }
}

void GCodeSender::emitStreamingEndedAndReset(StreamEndReason reason, QString description)
{
    m_device.reset();
    emit streamingEnded(reason, description);
    m_communicator->hardReset();
}

void GCodeSender::startSendingCommands()
{
    if (!m_device) {
        return;
    }

    m_startedSendingCommands = true;

    if (m_device->atEnd()) {
        // Empty stream, closing here
        finishStreaming();
        return;
    }

    m_wireController->switchWireOn();

    // Will send other commands in the commandSent callback
    readAndSendOneCommand();

    if (canSuccessfullyFinishStreaming()) {
        finishStreaming();
    }
}

void GCodeSender::finishStreaming()
{
    m_wireController->switchWireOff();
    m_device.reset(); // This is not tested (removing just to release resources, not strictly necessary)
    emit streamingEnded(StreamEndReason::Completed, tr("Success"));
}

bool GCodeSender::canSuccessfullyFinishStreaming() const
{
    // qDebug() << "Acks to receive:" << m_expectedAcks << "running?" << m_running;

    return m_device && m_device->atEnd() && m_running && m_expectedAcks == 0 &&
            m_machineStatusMonitor->state() == MachineState::Idle;
}

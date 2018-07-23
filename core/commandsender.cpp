#include "commandsender.h"
#include <QRegularExpression>

namespace {
    constexpr int grblBufferSize = 128;
    const QRegularExpression okRegExpr("^ok$", QRegularExpression::OptimizeOnFirstUsageOption);
    const QRegularExpression errorRegExpr("^error:([0-9]+)$", QRegularExpression::OptimizeOnFirstUsageOption);
}

CommandSenderListener::CommandSenderListener()
{
}

CommandSender::CommandSender(MachineCommunication* communicator)
    : m_communicator(communicator)
    , m_sentBytes()
    , m_resettingState(false)
{
    connect(m_communicator, &MachineCommunication::messageReceived, this, &CommandSender::messageReceived);
    connect(m_communicator, &MachineCommunication::portClosed, this, &CommandSender::resetState);
    connect(m_communicator, &MachineCommunication::portClosedWithError, this, &CommandSender::resetState);
    connect(m_communicator, &MachineCommunication::machineInitialized, this, &CommandSender::resetState);
}

bool CommandSender::sendCommand(QByteArray command, CommandCorrelationId correlationId, CommandSenderListener* listener)
{
    if (!validateAndFixCommand(command)) {
        return false;
    }

    if (listener && !m_listeners.contains(listener)) {
        m_listeners.insert(listener);
        connect(listener, &QObject::destroyed, this, &CommandSender::listenerDestroyed);
    }

    // Never send a new command if there are enqueued ones
    if (m_commandsToSend.isEmpty() && canSendCommand(command)) {
        enqueueAndSendCommand(correlationId, listener, command);
    } else {
        enqueueCommandToSend(correlationId, listener, command);
    }

    return true;
}

int CommandSender::pendingCommands() const
{
    return m_commandsToSend.size();
}

void CommandSender::messageReceived(QByteArray message)
{
    if (okRegExpr.match(message).hasMatch()) {
        dequeueSuccessfulCommand();
        dequeueCommandsToSend();
    } else {
        auto match = errorRegExpr.match(message);
        if (match.hasMatch()) {
            dequeueFailedCommand(match.captured(1).toInt());
            dequeueCommandsToSend();
        }
    }
}

void CommandSender::listenerDestroyed(QObject* obj)
{
    m_listeners.remove(obj);
}

void CommandSender::resetState()
{
    // This is to avoid nested calls to this function
    if (m_resettingState) {
        return;
    }

    m_resettingState = true;
    callReplyLostAndResetQueue(m_sentCommands, true);
    callReplyLostAndResetQueue(m_commandsToSend, false);

    m_sentBytes = 0;
    m_resettingState = false;
}

bool CommandSender::validateAndFixCommand(QByteArray& command)
{
    if (!command.endsWith('\n')) {
        command.append('\n');
    }

    if (command.size() > grblBufferSize) {
        return false;
    }

    if (command.indexOf('\n') != command.size() - 1) {
        return false;
    }

    if (command.indexOf('\r') != -1) {
        return false;
    }

    return true;
}

void CommandSender::enqueueAndSendCommand(CommandCorrelationId correlationId, CommandSenderListener* listener, QByteArray data)
{
    m_sentCommands.enqueue(Command{correlationId, listener, data.size()});
    m_sentBytes += data.size();
    m_communicator->writeData(data);

    if (validListener(listener)) {
        listener->commandSent(correlationId);
    }
}

void CommandSender::dequeueSuccessfulCommand()
{
    if (m_sentCommands.isEmpty()) {
        qInfo("Unexpected ok message received. Perhaps a command has been sent through the terminal?");
        return;
    }

    auto command = dequeueSentCommand();
    if (validListener(command.listener)) {
        command.listener->okReply(command.correlationId);
    }
}

void CommandSender::dequeueFailedCommand(int errorCode)
{
    if (m_sentCommands.isEmpty()) {
        qInfo("Unexpected error:%d message received. Perhaps a command has been sent through the terminal?", errorCode);
        return;
    }

    auto command = dequeueSentCommand();
    if (validListener(command.listener)) {
        command.listener->errorReply(command.correlationId, errorCode);
    }
}

void CommandSender::enqueueCommandToSend(CommandCorrelationId correlationId, CommandSenderListener* listener, QByteArray data)
{
    m_commandsToSend.enqueue(CommandToSend{correlationId, listener, data});
}

bool CommandSender::validListener(CommandSenderListener* listener) const
{
    return listener && m_listeners.contains(listener);
}

bool CommandSender::canSendCommand(const QByteArray& command)
{
    return m_sentBytes + command.size() <= grblBufferSize;
}

CommandSender::Command CommandSender::dequeueSentCommand()
{
    auto command = m_sentCommands.dequeue();
    m_sentBytes -= command.size;

    return command;
}

void CommandSender::dequeueCommandsToSend()
{
    while (!m_commandsToSend.isEmpty() && canSendCommand(m_commandsToSend.head().data)) {
        auto c = m_commandsToSend.dequeue();
        enqueueAndSendCommand(c.correlationId, c.listener, c.data);
    }
}

template <class QueueT>
void CommandSender::callReplyLostAndResetQueue(QueueT& queue, bool commandSent)
{
    for (auto& c: queue) {
        if (validListener(c.listener)) {
            c.listener->replyLost(c.correlationId, commandSent);
        }
    }

    queue.clear();
}

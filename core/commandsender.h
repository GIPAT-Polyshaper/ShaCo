#ifndef COMMANDSENDER_H
#define COMMANDSENDER_H

#include <QObject>
#include <QQueue>
#include <QSet>
#include "machinecommunication.h"

// TODO-TOMMY Write a class like CommandSender for immediate commands. It also need something like CommandSenderListener to receive replies (use virtual inheritance of QObject for both). This is to put all immediate commands in one place, removing the ones we have in MachineCommunication

using CommandCorrelationId = unsigned long int;

class CommandSenderListener : public QObject
{
    Q_OBJECT
public:
    explicit CommandSenderListener();

    virtual void commandSent(CommandCorrelationId correlationId) = 0;
    virtual void okReply(CommandCorrelationId correlationId) = 0;
    virtual void errorReply(CommandCorrelationId correlationId, int errorCode) = 0;
    virtual void replyLost(CommandCorrelationId correlationId, bool commandSent) = 0;
};

// This sends commands and expects an ok or error reply. Only use to send G-CODE commands. Immediate
// commands and other GRBL commands should be sent directly using MachineCommunication
class CommandSender : public QObject
{
    Q_OBJECT

    struct Command {
        CommandCorrelationId correlationId;
        CommandSenderListener* listener;
        int size;
    };

    struct CommandToSend {
        CommandCorrelationId correlationId;
        CommandSenderListener* listener;
        QByteArray data;
    };

public:
    explicit CommandSender(MachineCommunication* communicator);

    bool sendCommand(QByteArray command, CommandCorrelationId correlationId = 0, CommandSenderListener* listener = nullptr);
    // These are commands not sent yet. Those sent for which a reply has not been received yet are
    // not counted here
    int pendingCommands() const;

private slots:
    void messageReceived(QByteArray message);
    void listenerDestroyed(QObject* obj);
    void resetState();

private:
    bool validateAndFixCommand(QByteArray& command);
    void enqueueAndSendCommand(CommandCorrelationId correlationId, CommandSenderListener* listener, QByteArray data);
    void dequeueSuccessfulCommand();
    void dequeueFailedCommand(int errorCode);
    void enqueueCommandToSend(CommandCorrelationId correlationId, CommandSenderListener* listener, QByteArray data);
    bool validListener(CommandSenderListener* listener) const;
    bool canSendCommand(const QByteArray& command);
    Command dequeueSentCommand();
    void dequeueCommandsToSend();
    template <class QueueT> void callReplyLostAndResetQueue(QueueT& queue, bool commandSent);

    MachineCommunication* const m_communicator;
    QQueue<Command> m_sentCommands;
    QSet<QObject*> m_listeners;
    QQueue<CommandToSend> m_commandsToSend;
    int m_sentBytes;
    bool m_resettingState;
};

#endif // COMMANDSENDER_H

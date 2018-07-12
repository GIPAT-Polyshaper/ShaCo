#ifndef GCODESENDER_H
#define GCODESENDER_H

#include <functional>
#include <memory>
#include <QObject>
#include <QQueue>
#include <QString>
#include "commandsender.h"
#include "machinecommunication.h"
#include "machinestatusmonitor.h"
#include "wirecontroller.h"

// This is meant to be used only once: do not restart streaming when it ends
class GCodeSender : public CommandSenderListener
{
    Q_OBJECT

private:
    static const bool streamEndReasonRegistered;

public:
    enum class StreamEndReason {
        Completed,
        UserInterrupted,
        PortError,
        StreamError,
        MachineError
    };

public:
    explicit GCodeSender(MachineCommunication* communicator, CommandSender* commandSender, WireController* wireController, MachineStatusMonitor* machineStatusMonitor, std::unique_ptr<QIODevice>&& gcodeDevice);

    void commandSent(CommandCorrelationId correlationId) override;
    void okReply(CommandCorrelationId correlationId) override;
    void errorReply(CommandCorrelationId correlationId, int errorCode) override;
    void replyLost(CommandCorrelationId correlationId, bool commandSent) override;

public slots:
    void streamData();
    void interruptStreaming();

signals:
    void streamingStarted();
    void streamingResumed();
    // Class name needed because type registered with namespace
    void streamingEnded(GCodeSender::StreamEndReason reason, QString description);

private slots:
    void stateChanged(MachineState newState);

private:
    void readAndSendOneCommand();
    void emitStreamingEndedAndReset(StreamEndReason reason, QString description);
    void startSendingCommands();
    void finishStreaming();
    bool canSuccessfullyFinishStreaming() const;

    MachineCommunication* const m_communicator;
    CommandSender* const m_commandSender;
    WireController* const m_wireController;
    MachineStatusMonitor* const m_machineStatusMonitor;
    std::unique_ptr<QIODevice> m_device; // Non const to be reset when streaming ends
    bool m_running; // Machine switched to Run state
    int m_expectedAcks;
    bool m_startedSendingCommands; // We went Idle so we started streaming
};

Q_DECLARE_METATYPE(GCodeSender::StreamEndReason)

#endif // GCODESENDER_H

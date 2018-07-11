#ifndef GCODESENDER_H
#define GCODESENDER_H

#include <functional>
#include <memory>
#include <QObject>
#include <QQueue>
#include <QString>
#include "machinecommunication.h"
#include "machinestatusmonitor.h"
#include "wirecontroller.h"

class GCodeSender : public QObject
{
    Q_OBJECT

private:
    static const bool streamEndReasonRegistered;

public:
    enum class StreamEndReason {
        Completed,
        UserInterrupted,
        PortClosed,
        PortError,
        StreamError,
        MachineErrorReply
    };

public:
    // hardResetDelay is how much to wait at start after the hard reset (see
    // waitSomeTimeAtStartAfterHardReset test). Value is in milliseconds
    // idleWaitInterval is how much to wait at the end before starting to check that machine is idle
    // (see waitSomeTimeAtTheEndBeforeCheckingMachineIsIdle test). Value is in milliseconds
    explicit GCodeSender(int hardResetDelay, int idleWaitInterval, MachineCommunication* communicator, WireController* wireController, MachineStatusMonitor* machineStatusMonitor, std::unique_ptr<QIODevice>&& gcodeDevice);

public slots:
    void streamData();
    void interruptStreaming();

signals:
    void streamingStarted();
    void streamingResumed();
    // Class name needed because type registered with namespace
    void streamingEnded(GCodeSender::StreamEndReason reason, QString description);

private slots:
    void portClosedWithError();
    void portClosed();
    void messageReceived(QByteArray message);
    void stateChanged(MachineState newState);

private:
    void closeStream(GCodeSender::StreamEndReason reason, QString description);
    int bytesSentSinceLastAck() const;
    void waitWhileBufferFull(int requiredSpace);
    void emitStreamingEnded();
    void processEvents();

    const int m_hardResetDelay;
    const int m_idleWaitInterval;
    MachineCommunication* const m_communicator;
    WireController* const m_wireController;
    MachineStatusMonitor* const m_machineStatusMonitor;
    std::unique_ptr<QIODevice> m_device; // Non const to be reset when streaming ends
    QQueue<int> m_sentBytes;
    GCodeSender::StreamEndReason m_streamEndReason;
    QString m_streamEndDescription;
};

Q_DECLARE_METATYPE(GCodeSender::StreamEndReason)

#endif // GCODESENDER_H

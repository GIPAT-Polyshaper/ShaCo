#ifndef GCODESENDER_H
#define GCODESENDER_H

#include <functional>
#include <memory>
#include <QObject>
#include <QQueue>
#include <QString>
#include "machinecommunication.h"
#include "wirecontroller.h"

// NOTE We should perhaps switch the wire off while in pause? What should we do when stopped by the user? Or when there is a failure reading data from file?
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
    // Remember to move gcodeDevice to the same thread as this class before passing it here!!!
    explicit GCodeSender(MachineCommunication* communicator, WireController* wireController, std::unique_ptr<QIODevice>&& gcodeDevice);

    bool paused() const;

public slots:
    void streamData();
    void interruptStreaming();
    void pause();
    void resume();

signals:
    void streamingStarted();
    void streamingPaused();
    void streamingResumed();
    // Class name needed because type registered with namespace
    void streamingEnded(GCodeSender::StreamEndReason reason, QString description);

private slots:
    void portClosedWithError();
    void portClosed();
    void dataReceived(QByteArray data);

private:
    void closeStream(GCodeSender::StreamEndReason reason, QString description);
    int findEndCommandInPartialReply() const;
    int bytesSentSinceLastAck() const;
    void waitWhilePausedOrBufferFull(int requiredSpace);

    MachineCommunication* const m_communicator;
    WireController* const m_wireController;
    std::unique_ptr<QIODevice> m_device; // Non const to be reset when streaming ends
    QString m_partialReply;
    QQueue<int> m_sentBytes;
    bool m_paused;
};

Q_DECLARE_METATYPE(GCodeSender::StreamEndReason)

#endif // GCODESENDER_H

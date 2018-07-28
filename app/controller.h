#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <QObject>
#include <QThread>
#include <QSerialPortInfo>
#include <QUrl>
#include "worker.h"
#include "core/localshapesfinder.h"

class WorkerThread;

// TODO-TOMMY Add a test for this class??? If so we should make it template on all components to test connections and signal that are emitted
class Controller : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)
    Q_PROPERTY(bool streamingGCode READ streamingGCode NOTIFY streamingGCodeChanged)
    Q_PROPERTY(bool stoppingStreaming READ stoppingStreaming NOTIFY stoppingStreamingChanged)
    Q_PROPERTY(bool wireOn READ wireOn WRITE setWireOn NOTIFY wireOnChanged)
    Q_PROPERTY(float wireTemperature READ wireTemperature WRITE setWireTemperature NOTIFY wireTemperatureChanged)
    Q_PROPERTY(bool paused READ paused NOTIFY pausedChanged)
    Q_PROPERTY(bool senderCreated READ senderCreated NOTIFY senderCreatedChanged)

public:
    explicit Controller(QObject *parent = nullptr);
    virtual ~Controller();

    // Called by worker thread when all objects have been created
    void creationFinished();

    bool connected() const;
    bool streamingGCode() const;
    bool stoppingStreaming() const;
    bool wireOn() const;
    float wireTemperature() const;
    bool paused() const;
    bool senderCreated() const;

public slots:
    void sendLine(QByteArray line);
    void setGCodeFile(QUrl fileUrl);
    void setWireOn(bool wireOn);
    void setWireTemperature(float temperature);
    void startStreamingGCode();
    void stopStreaminGCode();
    void feedHold();
    void resumeFeedHold();

signals:
    void startedPortDiscovery();
    void portFound(QString machineName, QString firmwareVersion);
    void connectedChanged();
    void dataSent(QByteArray data);
    void dataReceived(QByteArray data);
    void portClosedWithError(QString reason);
    void portClosed();
    void streamingGCodeChanged();
    void stoppingStreamingChanged();
    void wireOnChanged();
    void wireTemperatureChanged();
    void streamingEndedWithError(QString reason);
    void pausedChanged();
    void senderCreatedChanged();

private slots:
    void gcodeSenderCreated(GCodeSender* sender);
    void signalPortFound(MachineInfo info);
    void signalPortClosedWithError(QString reason);
    void signalPortClosed();
    void streamingStarted();
    void streamingEnded(GCodeSender::StreamEndReason reason, QString description);

private:
    void setPaused();
    void unsetPaused();

    WorkerThread m_thread;
    bool m_connected;
    bool m_streamingGCode;
    bool m_stoppingStreaming;
    bool m_paused;
    bool m_senderCreated;
    LocalShapesFinder m_shapesFinder;
};

#endif // CONTROLLER_H

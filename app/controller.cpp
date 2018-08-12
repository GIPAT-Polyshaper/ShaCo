#include "controller.h"
#include <memory>
#include <QMetaObject>
#include <QDir>

Controller::Controller(QObject *parent)
    : QObject(parent)
    , m_thread(this)
    , m_connected(false)
    , m_streamingGCode(false)
    , m_stoppingStreaming(false)
    , m_paused(false)
    , m_senderCreated(false)
    , m_shapesFinder(QDir::homePath() + "/PolyShaper")
    , m_shapesModel(m_shapesFinder)
    , m_cutProgress(0)
{
    connect(&m_shapesFinder, &LocalShapesFinder::shapesUpdated, &m_shapesModel, &LocalShapesModel::shapesUpdated);
    connect(&m_cutTimer, &QTimer::timeout, this, &Controller::cutClockTimeout);
    m_cutTimer.setInterval(1000);
    m_cutTimer.setSingleShot(false);

    m_thread.start();
}

Controller::~Controller()
{
    m_thread.quit();
    m_thread.wait();
}

void Controller::creationFinished()
{
    connect(
        m_thread.worker()->portDiscoverer(), &PortDiscovery<QSerialPortInfo>::startedDiscoveringPort,
        this, &Controller::startedPortDiscovery
    );
    connect(
        m_thread.worker()->portDiscoverer(), &PortDiscovery<QSerialPortInfo>::portFound,
        this, &Controller::signalPortFound
    );
    connect(
        m_thread.worker()->portDiscoverer(), &PortDiscovery<QSerialPortInfo>::portFound,
        m_thread.worker()->machineCommunicator(), &MachineCommunication::portFound
    );
    connect(
        m_thread.worker()->machineCommunicator(), &MachineCommunication::dataSent,
        this, &Controller::dataSent
    );
    connect(
        m_thread.worker()->machineCommunicator(), &MachineCommunication::dataReceived,
        this, &Controller::dataReceived
    );
    connect(
        m_thread.worker()->machineCommunicator(), &MachineCommunication::portClosedWithError,
        this, &Controller::signalPortClosedWithError
    );
    connect(
        m_thread.worker()->machineCommunicator(), &MachineCommunication::portClosedWithError,
        m_thread.worker()->portDiscoverer(), &PortDiscovery<QSerialPortInfo>::start
    );
    connect(
        m_thread.worker()->machineCommunicator(), &MachineCommunication::portClosed,
        this, &Controller::signalPortClosed
    );
    connect(
        m_thread.worker()->machineCommunicator(), &MachineCommunication::portClosed,
        m_thread.worker()->portDiscoverer(), &PortDiscovery<QSerialPortInfo>::start
    );
    connect(
        m_thread.worker()->wireController(), &WireController::temperatureChanged,
        this, &Controller::wireTemperatureChanged
    );
    connect(
        m_thread.worker(), &Worker::gcodeSenderCreated,
        this, &Controller::gcodeSenderCreated
    );

    auto p = m_thread.worker()->portDiscoverer();
    QMetaObject::invokeMethod(p, [p](){ p->start(); });
}

bool Controller::connected() const
{
    return m_connected;
}

bool Controller::streamingGCode() const
{
    return m_streamingGCode;
}

bool Controller::stoppingStreaming() const
{
    return m_stoppingStreaming;
}

bool Controller::wireOn() const
{
    bool isWireOn = false;

    const auto p = m_thread.worker()->wireController();
    QMetaObject::invokeMethod(p, [p](){ return p->isWireOn(); }, Qt::BlockingQueuedConnection, &isWireOn);

    return isWireOn;
}

float Controller::wireTemperature() const
{
    float temperature = 0.0;

    const auto p = m_thread.worker()->wireController();
    QMetaObject::invokeMethod(p, [p](){ return p->temperature(); }, Qt::BlockingQueuedConnection, &temperature);

    return temperature;
}

bool Controller::paused() const
{
    return m_paused;
}

bool Controller::senderCreated() const
{
    return m_senderCreated;
}

QAbstractItemModel* Controller::localShapesModel()
{
    return &m_shapesModel;
}

qint64 Controller::cutProgress() const
{
    return m_cutProgress;
}

void Controller::sendLine(QByteArray line)
{
    auto p = m_thread.worker()->machineCommunicator();
    QMetaObject::invokeMethod(p, [p, line](){ p->writeLine(line); });
}

void Controller::setGCodeFile(QUrl fileUrl)
{
    auto p = m_thread.worker();
    QMetaObject::invokeMethod(p, [p, fileUrl](){ p->setGCodeFile(fileUrl); });
}

void Controller::setWireOn(bool wireOn)
{
    if (this->wireOn() == wireOn) {
        return;
    }

    auto p = m_thread.worker()->wireController();
    if (wireOn) {
        QMetaObject::invokeMethod(p, [p](){ p->switchWireOn(); });
    } else {
        QMetaObject::invokeMethod(p, [p](){ p->switchWireOff(); });
    }

    emit wireOnChanged();
}

void Controller::setWireTemperature(float temperature)
{
    if (wireTemperature() == temperature) {
        return;
    }

    auto p = m_thread.worker()->wireController();
    if (streamingGCode()) {
        QMetaObject::invokeMethod(p, [p, temperature](){ p->setRealTimeTemperature(temperature); });
    } else {
        QMetaObject::invokeMethod(p, [p, temperature](){ p->setTemperature(temperature); });
    }

    emit wireTemperatureChanged();
}

void Controller::startStreamingGCode()
{
    auto p = m_thread.worker()->gcodeSender();
    QMetaObject::invokeMethod(p, [p](){ p->streamData(); });

    unsetPaused();
}

void Controller::stopStreaminGCode()
{
    auto p = m_thread.worker()->gcodeSender();
    QMetaObject::invokeMethod(p, [p](){ p->interruptStreaming(); });

    unsetPaused();

    m_stoppingStreaming = true;
    emit stoppingStreamingChanged();
}

void Controller::feedHold()
{
    if (m_paused) {
        return;
    }

    auto p = m_thread.worker()->machineCommunicator();
    QMetaObject::invokeMethod(p, [p](){ p->feedHold(); });

    setPaused();
    pauseCutTimer();
}

void Controller::resumeFeedHold()
{
    if (!m_paused) {
        return;
    }

    auto p = m_thread.worker()->machineCommunicator();
    QMetaObject::invokeMethod(p, [p](){ p->resumeFeedHold(); });

    unsetPaused();
    resumeCutTimer();
}

void Controller::changeLocalShapesSort(QString sortBy)
{
    LocalShapesModel::SortCriterion s = LocalShapesModel::SortCriterion::Newest;

    if (sortBy == "newest") {
        s = LocalShapesModel::SortCriterion::Newest;
    } else if (sortBy == "a-z") {
        s = LocalShapesModel::SortCriterion::AZ;
    } else if (sortBy == "z-a") {
        s = LocalShapesModel::SortCriterion::ZA;
    }

    m_shapesModel.sortShapes(s);
}

void Controller::reloadShapes()
{
    m_shapesFinder.reload();
}

void Controller::gcodeSenderCreated(GCodeSender* sender)
{
    connect(sender, &GCodeSender::streamingStarted, this, &Controller::streamingStarted);
    connect(sender, &GCodeSender::streamingEnded, this, &Controller::streamingEnded);

    m_senderCreated = true;
    emit senderCreatedChanged();
}

void Controller::signalPortFound(MachineInfo info)
{
    m_connected = true;
    m_senderCreated = false;

    emit portFound(info.machineName(), info.partNumber(), info.serialNumber(), info.firmwareVersion());
    emit connectedChanged();
}

void Controller::signalPortClosedWithError(QString reason)
{
    m_connected = false;
    m_senderCreated = false;

    emit portClosedWithError(reason);
    emit connectedChanged();
}

void Controller::signalPortClosed()
{
    m_connected = false;

    emit connectedChanged();
}

void Controller::streamingStarted()
{
    m_streamingGCode = true;

    emit streamingGCodeChanged();

    initializeCutTimer();
}

void Controller::streamingEnded(GCodeSender::StreamEndReason reason, QString description)
{
    m_streamingGCode = false;

    if (reason != GCodeSender::StreamEndReason::Completed &&
        reason != GCodeSender::StreamEndReason::UserInterrupted) {
        emit streamingEndedWithError(description);
    }

    emit streamingGCodeChanged();

    if (m_stoppingStreaming) {
        m_stoppingStreaming = false;
        emit stoppingStreamingChanged();
    }

    m_senderCreated = false;
    emit senderCreatedChanged();

    m_cutTimer.stop();
}

void Controller::cutClockTimeout()
{
    m_cutProgress = m_cutStartTime.secsTo(QDateTime::currentDateTime());
    emit cutProgressChanged();
}

void Controller::setPaused()
{
    if (m_paused) {
        return;
    }

    m_paused = true;
    emit pausedChanged();
}

void Controller::unsetPaused()
{
    if (!m_paused) {
        return;
    }

    m_paused = false;
    emit pausedChanged();
}

void Controller::initializeCutTimer()
{
    m_cutStartTime = QDateTime::currentDateTime();
    m_cutTimer.start();
    m_cutProgress = 0;
    emit cutProgressChanged();
}

void Controller::pauseCutTimer()
{
    m_cutPauseStart = QDateTime::currentDateTime();
    m_cutTimer.stop();
}

void Controller::resumeCutTimer()
{
    auto pausedMillis = m_cutPauseStart.msecsTo(QDateTime::currentDateTime());
    m_cutStartTime = m_cutStartTime.addMSecs(pausedMillis);
    m_cutTimer.start();
}

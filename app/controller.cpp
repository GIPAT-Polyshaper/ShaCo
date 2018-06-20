#include "controller.h"
#include <memory>
#include <QFile>
#include <QMetaObject>

Controller::Controller(QObject *parent)
    : QObject(parent)
    , m_portThread()
    , m_portDiscoverer(new PortDiscovery<QSerialPortInfo>(QSerialPortInfo::availablePorts, [](QSerialPortInfo p){ return std::make_unique<SerialPort>(p); }, 100, 100))
    , m_machineCommunicator(new MachineCommunication())
    , m_wireController(new WireController(m_machineCommunicator))
    , m_gcodeSender(nullptr)
    , m_connected(false)
    , m_streamingGCode(false)
    , m_stoppingStreaming(false)
    , m_paused(false)
{
    moveToPortThread(m_portDiscoverer);
    moveToPortThread(m_machineCommunicator);
    moveToPortThread(m_wireController);

    connect(m_portDiscoverer, &PortDiscovery<QSerialPortInfo>::startedDiscoveringPort, this, &Controller::startedPortDiscovery);
    connect(m_portDiscoverer, &PortDiscovery<QSerialPortInfo>::portFound, this, &Controller::signalPortFound);
    connect(m_portDiscoverer, &PortDiscovery<QSerialPortInfo>::portFound, m_machineCommunicator, &MachineCommunication::portFound);
    connect(m_machineCommunicator, &MachineCommunication::dataSent, this, &Controller::dataSent);
    connect(m_machineCommunicator, &MachineCommunication::dataReceived, this, &Controller::dataReceived);
    connect(m_machineCommunicator, &MachineCommunication::portClosedWithError, this, &Controller::signalPortClosedWithError);
    connect(m_machineCommunicator, &MachineCommunication::portClosedWithError, m_portDiscoverer, &PortDiscovery<QSerialPortInfo>::start);
    connect(m_machineCommunicator, &MachineCommunication::portClosed, this, &Controller::signalPortClosed);
    connect(m_machineCommunicator, &MachineCommunication::portClosed, m_portDiscoverer, &PortDiscovery<QSerialPortInfo>::start);
    connect(m_wireController, &WireController::temperatureChanged, this, &Controller::wireTemperatureChanged);

    m_portThread.start();
    QMetaObject::invokeMethod(m_portDiscoverer, "start");
}

Controller::~Controller()
{
    m_portThread.quit();
    m_portThread.wait();
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

    QMetaObject::invokeMethod(m_wireController, [wireController = m_wireController](){ return wireController->isWireOn(); }, Qt::BlockingQueuedConnection, &isWireOn);

    return isWireOn;
}

float Controller::wireTemperature() const
{
    float temperature = 0.0;

    QMetaObject::invokeMethod(m_wireController, [wireController = m_wireController](){ return wireController->temperature(); }, Qt::BlockingQueuedConnection, &temperature);

    return temperature;
}

bool Controller::paused() const
{
    return m_paused;
}

void Controller::sendLine(QByteArray line)
{
    QMetaObject::invokeMethod(m_machineCommunicator, [communicator = m_machineCommunicator, line](){ communicator->writeLine(line); });
}

void Controller::setGCodeFile(QUrl fileUrl)
{
    auto file = std::make_unique<QFile>(fileUrl.toLocalFile());
    file->moveToThread(&m_portThread);

    if (m_gcodeSender) {
        QMetaObject::invokeMethod(m_gcodeSender, [sender = m_gcodeSender](){ sender->deleteLater(); });
    }

    m_gcodeSender = new GCodeSender(m_machineCommunicator, m_wireController, std::move(file));
    moveToPortThread(m_gcodeSender);

    connect(m_gcodeSender, &GCodeSender::streamingStarted, this, &Controller::streamingStarted);
    connect(m_gcodeSender, &GCodeSender::streamingEnded, this, &Controller::streamingEnded);
}

void Controller::setWireOn(bool wireOn)
{
    if (this->wireOn() == wireOn) {
        return;
    }

    if (wireOn) {
        QMetaObject::invokeMethod(m_wireController, [wireController = m_wireController](){ wireController->switchWireOn(); });
    } else {
        QMetaObject::invokeMethod(m_wireController, [wireController = m_wireController](){ wireController->switchWireOff(); });
    }

    emit wireOnChanged();
}

void Controller::setWireTemperature(float temperature)
{
    if (wireTemperature() == temperature) {
        return;
    }

    if (streamingGCode()) {
        QMetaObject::invokeMethod(m_wireController, [wireController = m_wireController, temperature](){ wireController->setRealTimeTemperature(temperature); });
    } else {
        QMetaObject::invokeMethod(m_wireController, [wireController = m_wireController, temperature](){ wireController->setTemperature(temperature); });
    }

    emit wireTemperatureChanged();
}

void Controller::startStreamingGCode()
{
    QMetaObject::invokeMethod(m_gcodeSender, [sender = m_gcodeSender](){ sender->streamData(); });

    unsetPaused();
}

void Controller::stopStreaminGCode()
{
    QMetaObject::invokeMethod(m_gcodeSender, [sender = m_gcodeSender](){ sender->interruptStreaming(); });

    unsetPaused();

    m_stoppingStreaming = true;
    emit stoppingStreamingChanged();
}

void Controller::feedHold()
{
    if (m_paused) {
        return;
    }

    QMetaObject::invokeMethod(m_machineCommunicator, [communicator = m_machineCommunicator](){ communicator->feedHold(); });

    setPaused();
}

void Controller::resumeFeedHold()
{
    if (!m_paused) {
        return;
    }

    QMetaObject::invokeMethod(m_machineCommunicator, [communicator = m_machineCommunicator](){ communicator->resumeFeedHold(); });

    unsetPaused();
}

void Controller::signalPortFound(MachineInfo info)
{
    m_connected = true;

    emit portFound(info.machineName(), info.firmwareVersion());
    emit connectedChanged();
}

void Controller::signalPortClosedWithError(QString reason)
{
    m_connected = false;

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
}

void Controller::moveToPortThread(QObject* obj)
{
    obj->moveToThread(&m_portThread);
    connect(&m_portThread, &QThread::finished, obj, &QObject::deleteLater);
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

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

void Controller::sendLine(QByteArray line)
{
    QMetaObject::invokeMethod(m_machineCommunicator, [communicator = m_machineCommunicator, line](){ communicator->writeLine(line); });
}

void Controller::setGCodeFile(QUrl fileUrl)
{
    auto file = std::make_unique<QFile>(fileUrl.toLocalFile());
    file->moveToThread(&m_portThread);

    m_gcodeSender = new GCodeSender(m_machineCommunicator, m_wireController, std::move(file));
    moveToPortThread(m_gcodeSender);

    connect(m_gcodeSender, &GCodeSender::streamingEnded, m_gcodeSender, &GCodeSender::deleteLater);
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

void Controller::streamingEnded(GCodeSender::StreamEndReason, QString)
{
    m_streamingGCode = false;

    // TODO-TOMMY QUI ALTRO SEGNALE SE STREAM FINISCE CON ERRORE

    emit streamingGCodeChanged();
}

void Controller::moveToPortThread(QObject* obj)
{
    obj->moveToThread(&m_portThread);
    connect(&m_portThread, &QThread::finished, obj, &QObject::deleteLater);
}

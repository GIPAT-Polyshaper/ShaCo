#include "worker.h"
#include <QFile>
#include <QMetaObject>
#include "controller.h"

WorkerThread::WorkerThread(Controller *controller)
    : m_controller(controller)
{

}

Worker* WorkerThread::worker() const
{
    return m_worker.get();
}

void WorkerThread::run()
{
    m_worker.reset(new Worker());

    QMetaObject::invokeMethod(m_controller, [controller = m_controller](){ controller->creationFinished(); });

    exec();

    m_worker.reset();
}

Worker::Worker()
    : m_portDiscoverer(new PortDiscovery<QSerialPortInfo>(QSerialPortInfo::availablePorts, [](QSerialPortInfo p){ return std::make_unique<SerialPort>(p); }, 1000, 300, 5, m_settings.characterSendDelayUs()))
    , m_machineCommunicator(new MachineCommunication(1000))
    , m_commandSender(new CommandSender(m_machineCommunicator.get()))
    , m_wireController(new WireController(m_machineCommunicator.get(), m_commandSender.get()))
    , m_statusMonitor(new MachineStatusMonitor(1000, 3000, m_machineCommunicator.get())) // polling every second
{
    m_wireController->setTemperature(m_settings.wireTemperature());
}

PortDiscovery<QSerialPortInfo>* Worker::portDiscoverer() const
{
    return m_portDiscoverer.get();
}

MachineCommunication* Worker::machineCommunicator() const
{
    return m_machineCommunicator.get();
}

CommandSender* Worker::commandSender() const
{
    return m_commandSender.get();
}

WireController* Worker::wireController() const
{
    return m_wireController.get();
}

MachineStatusMonitor* Worker::statusMonitor() const
{
    return m_statusMonitor.get();
}

GCodeSender* Worker::gcodeSender() const
{
    return m_gcodeSender.get();
}

void Worker::setGCodeFile(QUrl fileUrl)
{
    auto file = std::make_unique<QFile>(fileUrl.toLocalFile());

    // The old one, if existing, is deleted
    m_gcodeSender = std::make_unique<GCodeSender>(m_machineCommunicator.get(), m_commandSender.get(), m_wireController.get(), m_statusMonitor.get(), std::move(file));

    emit gcodeSenderCreated(m_gcodeSender.get());
}

void Worker::setCharacterSendDelayUs(unsigned long us)
{
    m_portDiscoverer->setCharacterSendDelayUs(us);
    m_machineCommunicator->setCharacterSendDelayUs(us);
}

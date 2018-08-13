#ifndef WORKER_H
#define WORKER_H

#include <memory>
#include <QThread>
#include <QUrl>
#include "core/commandsender.h"
#include "core/gcodesender.h"
#include "core/machinecommunication.h"
#include "core/machineinfo.h"
#include "core/machinestatusmonitor.h"
#include "core/portdiscovery.h"
#include "core/wirecontroller.h"
#include "settings.h"

class Controller;
class Worker;

// TODO-TOMMY This is not tested, see comment in controller.h
// This simply instantiates the worker class and then invokes a method in Controller to signal that
// object creation has finished
class WorkerThread : public QThread
{
    Q_OBJECT
public:
    explicit WorkerThread(Controller* controller);

    // Returns a valid pointer only while running. Call only after the thread has started and not
    // after thread has been asked to stop
    Worker* worker() const;

protected:
    void run() override;

private:
    Controller* const m_controller;
    std::unique_ptr<Worker> m_worker;
};

// This creates all objects that live in the worker thread and exposese their pointers. It lives
// in the worker thread, so that all its children also live in the same thread
class Worker : public QObject
{
    Q_OBJECT
public:
    explicit Worker();

    PortDiscovery<QSerialPortInfo>* portDiscoverer() const;
    MachineCommunication* machineCommunicator() const;
    CommandSender* commandSender() const;
    WireController* wireController() const;
    MachineStatusMonitor* statusMonitor() const;
    GCodeSender* gcodeSender() const;

public slots:
    void setGCodeFile(QUrl fileUrl);
    void updateCharacterSendDelayUs();

signals:
    void gcodeSenderCreated(GCodeSender* sender);

private:
    const Settings m_settings; // We only read settings on this thread
    std::unique_ptr<PortDiscovery<QSerialPortInfo>> m_portDiscoverer;
    std::unique_ptr<MachineCommunication> m_machineCommunicator;
    std::unique_ptr<CommandSender> m_commandSender;
    std::unique_ptr<WireController> m_wireController;
    std::unique_ptr<MachineStatusMonitor> m_statusMonitor;
    std::unique_ptr<GCodeSender> m_gcodeSender;
};

#endif // WORKER_H

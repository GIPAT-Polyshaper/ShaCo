#ifndef MACHINESTATUSMONITOR_H
#define MACHINESTATUSMONITOR_H

#include <QObject>
#include <QTimer>
#include "machinecommunication.h"
#include "machinestate.h"

class MachineStatusMonitor : public QObject
{
    Q_OBJECT
public:
    // statusPollingInterval is in milliseconds, as well as watchdogDelay (if no answer is received
    // within wathcdogDelay milliseconds, the port is closed)
    explicit MachineStatusMonitor(int statusPollingInterval, int watchdogDelay, MachineCommunication* communicator);

    MachineState state() const;

signals:
    void stateChanged(MachineState newState);

private slots:
    void machineInitialized();
    void sendStatusReportQuery();
    void messageReceived(QByteArray message);
    void portClosed();
    void watchdogTimerExpired();

private:
    void setNewState(MachineState newState);

    MachineCommunication* const m_communicator;
    QTimer m_timer;
    QTimer m_watchdog;
    MachineState m_state;
};

#endif // MACHINESTATUSMONITOR_H

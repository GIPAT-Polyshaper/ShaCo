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
    // statusPollingInterval is in milliseconds
    explicit MachineStatusMonitor(int statusPollingInterval, MachineCommunication* communicator);

    MachineState state() const;

signals:
    void stateChanged(MachineState newState);

private slots:
    void machineInitialized();
    void sendStatusReportQuery();
    void messageReceived(QByteArray message);
    void portClosed();

private:
    void setNewState(MachineState newState);

    MachineCommunication* const m_communicator;
    QTimer m_timer;
    MachineState m_state;
};

#endif // MACHINESTATUSMONITOR_H

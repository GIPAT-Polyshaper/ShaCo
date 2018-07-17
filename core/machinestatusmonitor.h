#ifndef MACHINESTATUSMONITOR_H
#define MACHINESTATUSMONITOR_H

#include <QObject>
#include <QTimer>
#include "machinecommunication.h"
#include "machinestate.h"

// TODO-TOMMY As this continuously asks for machine state, we might use this class to signal when the machien stops responding (e.g. if we don't get any reply for more than X seconds). We should also check for eroors when sending (especially after we implement a class like CommandSender for immeidate commands
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

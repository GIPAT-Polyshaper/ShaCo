#include "machinestatusmonitor.h"
#include <QRegularExpression>

namespace {
    const char statusReportQueryCommand = '?';

    const QRegularExpression statusRegExpr("^<(.*)>$", QRegularExpression::OptimizeOnFirstUsageOption);
}

MachineStatusMonitor::MachineStatusMonitor(int statusPollingInterval, MachineCommunication *communicator)
    : m_communicator(communicator)
    , m_state(MachineState::Unknown)
{
    m_timer.setInterval(statusPollingInterval);
    m_timer.setSingleShot(false);

    connect(m_communicator, &MachineCommunication::machineInitialized, this, &MachineStatusMonitor::machineInitialized);
    connect(m_communicator, &MachineCommunication::messageReceived, this, &MachineStatusMonitor::messageReceived);
    connect(m_communicator, &MachineCommunication::portClosed, this, &MachineStatusMonitor::portClosed);
    connect(m_communicator, &MachineCommunication::portClosedWithError, this, &MachineStatusMonitor::portClosed);
    connect(&m_timer, &QTimer::timeout, this, &MachineStatusMonitor::sendStatusReportQuery);
}

MachineState MachineStatusMonitor::state() const
{
    return m_state;
}

void MachineStatusMonitor::machineInitialized()
{
    setNewState(MachineState::Unknown);

    sendStatusReportQuery();

    m_timer.start();
}

void MachineStatusMonitor::sendStatusReportQuery()
{
    m_communicator->writeData(QByteArray(1, statusReportQueryCommand));
}

void MachineStatusMonitor::messageReceived(QByteArray message)
{
    const auto match = statusRegExpr.match(message);
    if (match.hasMatch()) {
        const auto parts = match.captured(1).toLatin1().split('|');

        setNewState(string2MachineState(parts[0]));
    }
}

void MachineStatusMonitor::portClosed()
{
    setNewState(MachineState::Unknown);
}

void MachineStatusMonitor::setNewState(MachineState newState)
{
    if (m_state != newState) {
        m_state = newState;
        emit stateChanged(m_state);
    }
}

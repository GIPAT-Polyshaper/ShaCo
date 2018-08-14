#ifndef MACHINESTATE_H
#define MACHINESTATE_H

#include <QByteArray>
#include <QMetaType>
#include <QString>

enum class MachineState {
    Idle,
    Run,
    Hold,
    Jog,
    Alarm,
    Door,
    Check,
    Home,
    Sleep,
    Unknown
};
Q_DECLARE_METATYPE(MachineState)

MachineState string2MachineState(const QByteArray& value);
QString machineState2String(MachineState state);

#endif // MACHINESTATE_H

#include "machinestate.h"

MachineState string2MachineState(const QByteArray& value)
{
    if (value == "Idle") {
        return MachineState::Idle;
    } else if (value == "Run") {
        return MachineState::Run;
    } else if (value == "Hold") {
        return MachineState::Hold;
    } else if (value == "Jog") {
        return MachineState::Jog;
    } else if (value == "Alarm") {
        return MachineState::Alarm;
    } else if (value == "Door") {
        return MachineState::Door;
    } else if (value == "Check") {
        return MachineState::Check;
    } else if (value == "Home") {
        return MachineState::Home;
    } else if (value == "Sleep") {
        return MachineState::Sleep;
    }

    return MachineState::Unknown;
}

QString machineState2String(MachineState state)
{
    switch(state) {
        case MachineState::Idle:
            return "Idle";
        case MachineState::Run:
            return "Run";
        case MachineState::Hold:
            return "Hold";
        case MachineState::Jog:
            return "Jog";
        case MachineState::Alarm:
            return "Alarm";
        case MachineState::Door:
            return "Door";
        case MachineState::Check:
            return "Check";
        case MachineState::Home:
            return "Home";
        case MachineState::Sleep:
            return "Sleep";
        case MachineState::Unknown:
            return "Unknown";
    }

    // To prevent compiler warnings
    return "Unknown";
}

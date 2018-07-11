#include <QtTest>
#include "core/machinestate.h"

class MachineStateTest : public QObject
{
    Q_OBJECT

public:
    MachineStateTest();

private Q_SLOTS:
    void testConversionFromString();
    void testConversionToString();
};

MachineStateTest::MachineStateTest()
{
}

void MachineStateTest::testConversionFromString()
{
    QCOMPARE(string2MachineState("Idle"), MachineState::Idle);
    QCOMPARE(string2MachineState("Run"), MachineState::Run);
    QCOMPARE(string2MachineState("Hold"), MachineState::Hold);
    QCOMPARE(string2MachineState("Jog"), MachineState::Jog);
    QCOMPARE(string2MachineState("Alarm"), MachineState::Alarm);
    QCOMPARE(string2MachineState("Door"), MachineState::Door);
    QCOMPARE(string2MachineState("Check"), MachineState::Check);
    QCOMPARE(string2MachineState("Home"), MachineState::Home);
    QCOMPARE(string2MachineState("Sleep"), MachineState::Sleep);
    QCOMPARE(string2MachineState("Unknown"), MachineState::Unknown);
    QCOMPARE(string2MachineState("Anything Else"), MachineState::Unknown);
}

void MachineStateTest::testConversionToString()
{
    QCOMPARE(machineState2String(MachineState::Idle), "Idle");
    QCOMPARE(machineState2String(MachineState::Run), "Run");
    QCOMPARE(machineState2String(MachineState::Hold), "Hold");
    QCOMPARE(machineState2String(MachineState::Jog), "Jog");
    QCOMPARE(machineState2String(MachineState::Alarm), "Alarm");
    QCOMPARE(machineState2String(MachineState::Door), "Door");
    QCOMPARE(machineState2String(MachineState::Check), "Check");
    QCOMPARE(machineState2String(MachineState::Home), "Home");
    QCOMPARE(machineState2String(MachineState::Sleep), "Sleep");
    QCOMPARE(machineState2String(MachineState::Unknown), "Unknown");
}

QTEST_APPLESS_MAIN(MachineStateTest)

#include "machinestate_test.moc"

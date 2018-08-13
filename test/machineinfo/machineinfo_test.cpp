#include <QtTest>
#include "core/machineinfo.h"

class MachineInfoTest : public QObject
{
    Q_OBJECT

public:
    MachineInfoTest();

private Q_SLOTS:
    void createFromStringReturnsInvalidMachineInfoForInvalidData();
    void createFromStringReturnsMachineInfo();
    void doNotSaveBracketsInFirmwareVersion();
    void createFromStringWhenMachineIsOranje();
    void createFromStringWhenMachineIsAzul();
};

MachineInfoTest::MachineInfoTest()
{
}

void MachineInfoTest::createFromStringReturnsInvalidMachineInfoForInvalidData()
{
    auto info = MachineInfo::createFromString("invalid data");

    QVERIFY(!info);
}

void MachineInfoTest::createFromStringReturnsMachineInfo()
{
    auto info = MachineInfo::createFromString("bla bla[PolyShaper MachineName][pn123 sn456 789]");

    QCOMPARE(info->machineName(), "MachineName");
    QCOMPARE(info->partNumber(), "pn123");
    QCOMPARE(info->serialNumber(), "sn456");
    QCOMPARE(info->firmwareVersion(), "789");
    QCOMPARE(info->maxWireTemperature(), 100.0f);
}

void MachineInfoTest::doNotSaveBracketsInFirmwareVersion()
{
    auto info = MachineInfo::createFromString("bla bla[PolyShaper MachineName][pn123 sn456 789][ffdsa]");

    QCOMPARE(info->machineName(), "MachineName");
    QCOMPARE(info->partNumber(), "pn123");
    QCOMPARE(info->serialNumber(), "sn456");
    QCOMPARE(info->firmwareVersion(), "789");
    QCOMPARE(info->maxWireTemperature(), 100.0f);
}

void MachineInfoTest::createFromStringWhenMachineIsOranje()
{
    auto info = MachineInfo::createFromString("bla bla[PolyShaper Oranje][pn123 sn456 789]");

    QCOMPARE(info->machineName(), "Oranje");
    QCOMPARE(info->partNumber(), "pn123");
    QCOMPARE(info->serialNumber(), "sn456");
    QCOMPARE(info->firmwareVersion(), "789");
    QCOMPARE(info->maxWireTemperature(), 35.0f);
}

void MachineInfoTest::createFromStringWhenMachineIsAzul()
{
    auto info = MachineInfo::createFromString("bla bla[PolyShaper Azul][pn123 sn456 789]");

    QCOMPARE(info->machineName(), "Azul");
    QCOMPARE(info->partNumber(), "pn123");
    QCOMPARE(info->serialNumber(), "sn456");
    QCOMPARE(info->firmwareVersion(), "789");
    QCOMPARE(info->maxWireTemperature(), 75.0f);
}

QTEST_APPLESS_MAIN(MachineInfoTest)

#include "machineinfo_test.moc"

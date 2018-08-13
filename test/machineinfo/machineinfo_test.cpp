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
}

void MachineInfoTest::doNotSaveBracketsInFirmwareVersion()
{
    auto info = MachineInfo::createFromString("bla bla[PolyShaper MachineName][pn123 sn456 789][ffdsa]");

    QCOMPARE(info->machineName(), "MachineName");
    QCOMPARE(info->partNumber(), "pn123");
    QCOMPARE(info->serialNumber(), "sn456");
    QCOMPARE(info->firmwareVersion(), "789");
}

QTEST_APPLESS_MAIN(MachineInfoTest)

#include "machineinfo_test.moc"

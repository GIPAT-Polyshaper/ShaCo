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
};

MachineInfoTest::MachineInfoTest()
{
}

void MachineInfoTest::createFromStringReturnsInvalidMachineInfoForInvalidData()
{
    auto info = MachineInfo::createFromString("invalid data");

    QVERIFY(!info.isValid());
}

void MachineInfoTest::createFromStringReturnsMachineInfo()
{
    auto info = MachineInfo::createFromString("bla bla[PolyShaper VERSION][X.Y.Z]");

    QCOMPARE(info.machineName(), "VERSION");
    QCOMPARE(info.firmwareVersion(), "X.Y.Z");
}

QTEST_APPLESS_MAIN(MachineInfoTest)

#include "machineinfo_test.moc"

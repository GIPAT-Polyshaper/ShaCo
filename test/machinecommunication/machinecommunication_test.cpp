#include <QString>
#include <QtTest>
#include <QCoreApplication>

class MachineCommunicationTest : public QObject
{
    Q_OBJECT

public:
    MachineCommunicationTest();

private Q_SLOTS:
    void testCase1();
};

MachineCommunicationTest::MachineCommunicationTest()
{
}

void MachineCommunicationTest::testCase1()
{
    QVERIFY2(true, "Failure");
}

QTEST_GUILESS_MAIN(MachineCommunicationTest)

#include "machinecommunication_test.moc"

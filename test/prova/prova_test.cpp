#include <QtTest/QtTest>
#include "core/prova.h"

class ProvaTest: public QObject
{
    Q_OBJECT
private slots:
    void aTest() {
        QVERIFY(testAdd(10, 20) == 30);
    }
};

QTEST_MAIN(ProvaTest)
#include "prova_test.moc"

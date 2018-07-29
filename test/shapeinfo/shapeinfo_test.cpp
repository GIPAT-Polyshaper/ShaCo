#include <memory>
#include <QByteArray>
#include <QDateTime>
#include <QStringList>
#include <QTemporaryFile>
#include <QtTest>
#include "core/shapeinfo.h"

class ShapeInfoTest : public QObject
{
    Q_OBJECT

public:
    ShapeInfoTest();

private:
    std::unique_ptr<QTemporaryFile> writeJsonToFile(QByteArray content);

private Q_SLOTS:
    void createInvalidShapeIfFileDoesNotExists();
    void createShapeFromFile();
    void createInvalidShapeIfFileHasInvalidFormat();
    void createInvalidShapeIfVersionIsUnknown();
    void createInvalidShapeIfAnyFieldIsMissing();
    void createInvalidShapeIfAnyFielsHasInvalidType();
    void createInvalidShapeIfCreationDateFormatIsInvalid();
    void createInvalidShapeIfDurationIsNegative();
    void createInvalidShapeIfDurationIsNotInt();
};

ShapeInfoTest::ShapeInfoTest()
{
}

std::unique_ptr<QTemporaryFile> ShapeInfoTest::writeJsonToFile(QByteArray content)
{
    auto file = std::make_unique<QTemporaryFile>();
    if (!file->open()) {
        throw QString("CANNOT OPEN TEMPORARY FILE!!!");
    }
    file->write(content);
    file->close();

    return file;
}

void ShapeInfoTest::createInvalidShapeIfFileDoesNotExists()
{
    // This file should not exists...
    auto info = ShapeInfo::createFromFile("jdsflkjhesriohvuiehhrewiuq u3982hns.ffgsaf");

    QVERIFY(!info.isValid());
}

void ShapeInfoTest::createShapeFromFile()
{
    QByteArray content = R"(
{
  "version": 1,
  "name": "sandman",
  "svgFilename": "/home/tommy/PolyShaper/polyshaper-000.svg",
  "square": true,
  "machineType": "PolyShaperOranje",
  "drawToolpath": true,
  "margin": 10.0,
  "generatedBy": "2DPlugin",
  "creationTime": "2018-07-26T22:56:56.931242",
  "flatness": 0.001,
  "workpieceDimX": 400.0,
  "workpieceDimY": 450.0,
  "autoClosePath": true,
  "duration": 81,
  "pointsInsideWorkpiece": true,
  "speed": 1000.0,
  "gcodeFilename": "/home/tommy/PolyShaper/polyshaper-000.gcode"
})";
    auto file = writeJsonToFile(content);

    auto info = ShapeInfo::createFromFile(file->fileName());

    QVERIFY(info.isValid());
    QCOMPARE(info.version(), 1u);
    QCOMPARE(info.name(), "sandman");
    QCOMPARE(info.svgFilename(), "/home/tommy/PolyShaper/polyshaper-000.svg");
    QCOMPARE(info.square(), true);
    QCOMPARE(info.machineType(), "PolyShaperOranje");
    QCOMPARE(info.drawToolpath(), true);
    QCOMPARE(info.margin(), 10.0);
    QCOMPARE(info.generatedBy(), "2DPlugin");
    QCOMPARE(info.creationTime(), QDateTime::fromString("2018-07-26T22:56:56.931242", Qt::ISODateWithMs));
    QCOMPARE(info.flatness(), 0.001);
    QCOMPARE(info.workpieceDimX(), 400.0);
    QCOMPARE(info.workpieceDimY(), 450.0);
    QCOMPARE(info.autoClosePath(), true);
    QCOMPARE(info.duration(), 81u);
    QCOMPARE(info.pointsInsideWorkpiece(), true);
    QCOMPARE(info.speed(), 1000.0);
    QCOMPARE(info.gcodeFilename(), "/home/tommy/PolyShaper/polyshaper-000.gcode");
}

void ShapeInfoTest::createInvalidShapeIfFileHasInvalidFormat()
{
    QByteArray content = "An Invalid JSON!!!";
    auto file = writeJsonToFile(content);

    auto info = ShapeInfo::createFromFile(file->fileName());

    QVERIFY(!info.isValid());
}

void ShapeInfoTest::createInvalidShapeIfVersionIsUnknown()
{
    QByteArray content = R"(
{
  "version": 999999,
  "name": "sandman",
  "svgFilename": "/home/tommy/PolyShaper/polyshaper-000.svg",
  "square": true,
  "machineType": "PolyShaperOranje",
  "drawToolpath": true,
  "margin": 10.0,
  "generatedBy": "2DPlugin",
  "creationTime": "2018-07-26T22:56:56.931242",
  "flatness": 0.001,
  "workpieceDimX": 400.0,
  "workpieceDimY": 450.0,
  "autoClosePath": true,
  "duration": 81,
  "pointsInsideWorkpiece": true,
  "speed": 1000.0,
  "gcodeFilename": "/home/tommy/PolyShaper/polyshaper-000.gcode"
})";
    auto file = writeJsonToFile(content);

    auto info = ShapeInfo::createFromFile(file->fileName());

    QVERIFY(!info.isValid());
}

void ShapeInfoTest::createInvalidShapeIfAnyFieldIsMissing()
{
    QList<QByteArray> fields{
        R"("name": "sandman")",
        R"("svgFilename": "/home/tommy/PolyShaper/polyshaper-000.svg")",
        R"("square": true)",
        R"("machineType": "PolyShaperOranje")",
        R"("drawToolpath": true)",
        R"("margin": 10.0)",
        R"("generatedBy": "2DPlugin")",
        R"("creationTime": "2018-07-26T22:56:56.931242")",
        R"("flatness": 0.001)",
        R"("workpieceDimX": 400.0)",
        R"("workpieceDimY": 450.0)",
        R"("autoClosePath": true)",
        R"("duration": 81)",
        R"("pointsInsideWorkpiece": true)",
        R"("speed": 1000.0)",
        R"("gcodeFilename": "/home/tommy/PolyShaper/polyshaper-000.gcode")"
    };

    // Skipping one row at a time
    for (auto i = 0; i < fields.size(); ++i){
        QByteArray content = R"({"version":1)";
        for (auto j = 0; j < fields.size(); ++j) {
            if (j != i) {
                content += "," + fields[j];
            }
        }
        content += "}";
        auto file = writeJsonToFile(content);

        auto info = ShapeInfo::createFromFile(file->fileName());
        QVERIFY(!info.isValid());
    }
}

void ShapeInfoTest::createInvalidShapeIfAnyFielsHasInvalidType()
{
    QList<QByteArray> validFields{
        R"("name": "sandman")",
        R"("svgFilename": "/home/tommy/PolyShaper/polyshaper-000.svg")",
        R"("square": true)",
        R"("machineType": "PolyShaperOranje")",
        R"("drawToolpath": true)",
        R"("margin": 10.0)",
        R"("generatedBy": "2DPlugin")",
        R"("creationTime": "2018-07-26T22:56:56.931242")",
        R"("flatness": 0.001)",
        R"("workpieceDimX": 400.0)",
        R"("workpieceDimY": 450.0)",
        R"("autoClosePath": true)",
        R"("duration": 81)",
        R"("pointsInsideWorkpiece": true)",
        R"("speed": 1000.0)",
        R"("gcodeFilename": "/home/tommy/PolyShaper/polyshaper-000.gcode")"
    };

    QList<QByteArray> invalidFields{
        R"("name": 1)",
        R"("svgFilename": 1)",
        R"("square": 1)",
        R"("machineType": 1)",
        R"("drawToolpath": 1)",
        R"("margin": "10.0")",
        R"("generatedBy": 1)",
        R"("creationTime": 1)",
        R"("flatness": "0.001")",
        R"("workpieceDimX": "400.0")",
        R"("workpieceDimY": "450.0")",
        R"("autoClosePath": 1)",
        R"("duration": "81")",
        R"("pointsInsideWorkpiece": 1)",
        R"("speed": "1000.0")",
        R"("gcodeFilename": 1)"
    };

    // Safety check
    QCOMPARE(validFields.size(), invalidFields.size());

    // Substituting one row at a time
    for (auto i = 0; i < validFields.size(); ++i){
        QByteArray content = R"({"version":1)";
        for (auto j = 0; j < invalidFields.size(); ++j) {
            content += "," + (j == i ? invalidFields[j] : validFields[j]);
        }
        content += "}";
        auto file = writeJsonToFile(content);

        auto info = ShapeInfo::createFromFile(file->fileName());
        QVERIFY(!info.isValid());
    }

}

void ShapeInfoTest::createInvalidShapeIfCreationDateFormatIsInvalid()
{
    QByteArray content = R"(
{
  "version": 1,
  "name": "sandman",
  "svgFilename": "/home/tommy/PolyShaper/polyshaper-000.svg",
  "square": true,
  "machineType": "PolyShaperOranje",
  "drawToolpath": true,
  "margin": 10.0,
  "generatedBy": "2DPlugin",
  "creationTime": "Invalid format",
  "flatness": 0.001,
  "workpieceDimX": 400.0,
  "workpieceDimY": 450.0,
  "autoClosePath": true,
  "duration": 81,
  "pointsInsideWorkpiece": true,
  "speed": 1000.0,
  "gcodeFilename": "/home/tommy/PolyShaper/polyshaper-000.gcode"
})";
    auto file = writeJsonToFile(content);

    auto info = ShapeInfo::createFromFile(file->fileName());

    QVERIFY(!info.isValid());
}

void ShapeInfoTest::createInvalidShapeIfDurationIsNegative()
{
    QByteArray content = R"(
{
  "version": 1,
  "name": "sandman",
  "svgFilename": "/home/tommy/PolyShaper/polyshaper-000.svg",
  "square": true,
  "machineType": "PolyShaperOranje",
  "drawToolpath": true,
  "margin": 10.0,
  "generatedBy": "2DPlugin",
  "creationTime": "2018-07-26T22:56:56.931242",
  "flatness": 0.001,
  "workpieceDimX": 400.0,
  "workpieceDimY": 450.0,
  "autoClosePath": true,
  "duration": -81,
  "pointsInsideWorkpiece": true,
  "speed": 1000.0,
  "gcodeFilename": "/home/tommy/PolyShaper/polyshaper-000.gcode"
})";
    auto file = writeJsonToFile(content);

    auto info = ShapeInfo::createFromFile(file->fileName());

    QVERIFY(!info.isValid());
}

void ShapeInfoTest::createInvalidShapeIfDurationIsNotInt()
{
    QByteArray content = R"(
{
  "version": 1,
  "name": "sandman",
  "svgFilename": "/home/tommy/PolyShaper/polyshaper-000.svg",
  "square": true,
  "machineType": "PolyShaperOranje",
  "drawToolpath": true,
  "margin": 10.0,
  "generatedBy": "2DPlugin",
  "creationTime": "2018-07-26T22:56:56.931242",
  "flatness": 0.001,
  "workpieceDimX": 400.0,
  "workpieceDimY": 450.0,
  "autoClosePath": true,
  "duration": 81.7,
  "pointsInsideWorkpiece": true,
  "speed": 1000.0,
  "gcodeFilename": "/home/tommy/PolyShaper/polyshaper-000.gcode"
})";
    auto file = writeJsonToFile(content);

    auto info = ShapeInfo::createFromFile(file->fileName());

    QVERIFY(!info.isValid());
}

QTEST_GUILESS_MAIN(ShapeInfoTest)

#include "shapeinfo_test.moc"

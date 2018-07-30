#include <memory>
#include <QByteArray>
#include <QDateTime>
#include <QFileInfo>
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
    void openJsonFileAsText();
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
  "svgFilename": "polyshaper-000.svg",
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
  "gcodeFilename": "polyshaper-000.gcode"
})";
    auto file = writeJsonToFile(content);

    auto info = ShapeInfo::createFromFile(file->fileName());

    QVERIFY(info.isValid());
    QCOMPARE(info.version(), 1u);
    QCOMPARE(info.path(), QFileInfo(file->fileName()).canonicalPath());
    QCOMPARE(info.psjFilename(), QFileInfo(file->fileName()).fileName());
    QCOMPARE(info.name(), "sandman");
    QCOMPARE(info.svgFilename(), "polyshaper-000.svg");
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
    QCOMPARE(info.gcodeFilename(), "polyshaper-000.gcode");
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
  "svgFilename": "polyshaper-000.svg",
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
  "gcodeFilename": "polyshaper-000.gcode"
})";
    auto file = writeJsonToFile(content);

    auto info = ShapeInfo::createFromFile(file->fileName());

    QVERIFY(!info.isValid());
}

void ShapeInfoTest::createInvalidShapeIfAnyFieldIsMissing()
{
    QList<QByteArray> fields{
        R"("name": "sandman")",
        R"("svgFilename": "polyshaper-000.svg")",
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
        R"("gcodeFilename": "polyshaper-000.gcode")"
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
        R"("svgFilename": "polyshaper-000.svg")",
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
        R"("gcodeFilename": "polyshaper-000.gcode")"
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
  "svgFilename": "polyshaper-000.svg",
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
  "gcodeFilename": "polyshaper-000.gcode"
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
  "svgFilename": "polyshaper-000.svg",
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
  "gcodeFilename": "polyshaper-000.gcode"
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
  "svgFilename": "polyshaper-000.svg",
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
  "gcodeFilename": "polyshaper-000.gcode"
})";
    auto file = writeJsonToFile(content);

    auto info = ShapeInfo::createFromFile(file->fileName());

    QVERIFY(!info.isValid());
}

void ShapeInfoTest::openJsonFileAsText()
{
    // Here we test both \n and \r\n line terminators
    QByteArray contentUnix = "\n\
{\n\
  \"version\": 1,\n\
  \"name\": \"sandman\",\n\
  \"svgFilename\": \"polyshaper-000.svg\",\n\
  \"square\": true,\n\
  \"machineType\": \"PolyShaperOranje\",\n\
  \"drawToolpath\": true,\n\
  \"margin\": 10.0,\n\
  \"generatedBy\": \"2DPlugin\",\n\
  \"creationTime\": \"2018-07-26T22:56:56.931242\",\n\
  \"flatness\": 0.001,\n\
  \"workpieceDimX\": 400.0,\n\
  \"workpieceDimY\": 450.0,\n\
  \"autoClosePath\": true,\n\
  \"duration\": 81,\n\
  \"pointsInsideWorkpiece\": true,\n\
  \"speed\": 1000.0,\n\
  \"gcodeFilename\": \"polyshaper-000.gcode\"\n\
}";
    QByteArray contentDos = "\r\n\
{\r\n\
  \"version\": 1,\r\n\
  \"name\": \"sandman\",\r\n\
  \"svgFilename\": \"polyshaper-000.svg\",\r\n\
  \"square\": true,\r\n\
  \"machineType\": \"PolyShaperOranje\",\r\n\
  \"drawToolpath\": true,\r\n\
  \"margin\": 10.0,\r\n\
  \"generatedBy\": \"2DPlugin\",\r\n\
  \"creationTime\": \"2018-07-26T22:56:56.931242\",\r\n\
  \"flatness\": 0.001,\r\n\
  \"workpieceDimX\": 400.0,\r\n\
  \"workpieceDimY\": 450.0,\r\n\
  \"autoClosePath\": true,\r\n\
  \"duration\": 81,\r\n\
  \"pointsInsideWorkpiece\": true,\r\n\
  \"speed\": 1000.0,\r\n\
  \"gcodeFilename\": \"polyshaper-000.gcode\"\r\n\
}";

    auto fileUnix = writeJsonToFile(contentUnix);
    auto fileDos = writeJsonToFile(contentDos);

    auto infoUnix = ShapeInfo::createFromFile(fileUnix->fileName());
    QVERIFY(infoUnix.isValid());
    QCOMPARE(infoUnix.version(), 1u);
    QCOMPARE(infoUnix.path(), QFileInfo(fileUnix->fileName()).canonicalPath());
    QCOMPARE(infoUnix.psjFilename(), QFileInfo(fileUnix->fileName()).fileName());
    QCOMPARE(infoUnix.name(), "sandman");
    QCOMPARE(infoUnix.svgFilename(), "polyshaper-000.svg");
    QCOMPARE(infoUnix.square(), true);
    QCOMPARE(infoUnix.machineType(), "PolyShaperOranje");
    QCOMPARE(infoUnix.drawToolpath(), true);
    QCOMPARE(infoUnix.margin(), 10.0);
    QCOMPARE(infoUnix.generatedBy(), "2DPlugin");
    QCOMPARE(infoUnix.creationTime(), QDateTime::fromString("2018-07-26T22:56:56.931242", Qt::ISODateWithMs));
    QCOMPARE(infoUnix.flatness(), 0.001);
    QCOMPARE(infoUnix.workpieceDimX(), 400.0);
    QCOMPARE(infoUnix.workpieceDimY(), 450.0);
    QCOMPARE(infoUnix.autoClosePath(), true);
    QCOMPARE(infoUnix.duration(), 81u);
    QCOMPARE(infoUnix.pointsInsideWorkpiece(), true);
    QCOMPARE(infoUnix.speed(), 1000.0);
    QCOMPARE(infoUnix.gcodeFilename(), "polyshaper-000.gcode");

    auto infoDos = ShapeInfo::createFromFile(fileDos->fileName());
    QVERIFY(infoDos.isValid());
    QCOMPARE(infoDos.version(), 1u);
    QCOMPARE(infoDos.path(), QFileInfo(fileDos->fileName()).canonicalPath());
    QCOMPARE(infoDos.psjFilename(), QFileInfo(fileDos->fileName()).fileName());
    QCOMPARE(infoDos.name(), "sandman");
    QCOMPARE(infoDos.svgFilename(), "polyshaper-000.svg");
    QCOMPARE(infoDos.square(), true);
    QCOMPARE(infoDos.machineType(), "PolyShaperOranje");
    QCOMPARE(infoDos.drawToolpath(), true);
    QCOMPARE(infoDos.margin(), 10.0);
    QCOMPARE(infoDos.generatedBy(), "2DPlugin");
    QCOMPARE(infoDos.creationTime(), QDateTime::fromString("2018-07-26T22:56:56.931242", Qt::ISODateWithMs));
    QCOMPARE(infoDos.flatness(), 0.001);
    QCOMPARE(infoDos.workpieceDimX(), 400.0);
    QCOMPARE(infoDos.workpieceDimY(), 450.0);
    QCOMPARE(infoDos.autoClosePath(), true);
    QCOMPARE(infoDos.duration(), 81u);
    QCOMPARE(infoDos.pointsInsideWorkpiece(), true);
    QCOMPARE(infoDos.speed(), 1000.0);
    QCOMPARE(infoDos.gcodeFilename(), "polyshaper-000.gcode");
}

QTEST_GUILESS_MAIN(ShapeInfoTest)

#include "shapeinfo_test.moc"

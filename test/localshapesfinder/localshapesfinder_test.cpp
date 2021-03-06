#include <memory>
#include <QFile>
#include <QFileSystemWatcher>
#include <QSignalSpy>
#include <QSysInfo>
#include <QTemporaryDir>
#include <QtTest>
#include "core/localshapesfinder.h"

class LocalShapesFinderTest : public QObject
{
    Q_OBJECT

public:
    LocalShapesFinderTest();

private:
    std::unique_ptr<QTemporaryDir> m_dir;
    QString m_curPath;
    void createFiles(unsigned int startIndex, unsigned int count, QString ext = "psj", QByteArray creator = "2DPlugin");
    void createFilesInPath(QString path, unsigned int startIndex, unsigned int count, QString ext = "psj", QByteArray creator = "2DPlugin");

private Q_SLOTS:
    void init();
    void cleanup();

    void testQFileSysteWatcherUsage();
    void testCreatePsjFiles();
    void loadFilesFromDirectory();
    void onlyLoadFilesWithPsjExtension();
    void skipInvalidPsjFiles();
    void skipPsjFilesWithoutACorrespondingGCodeFile();
    void skipPsjFilesWithoutACorrespondingSvgFile();
    void emitSignalWhenShapesAreFirstFound();
    void onlyLoadNewFilesWhenDirectoryChanges();
    void emitSignalWhenNewShapesAreFound();
    void removeShapesThatNoLongerExist();
    void emitSignalWhenShapesAreRemoved();
    void removeAllShapesIfDirectoryIsRemoved();
    void doNotEmitSignalIfDirectoryRemovedAndThereIsNoShape();
    void loadFilesFromDirAtStart();
    void doNotEmitSignalsIfThereIsNoChange();
    void createDirectoryIfNotExistingAtStart();
    void whenRescanIsCalledByHandReloadEverythingFromTheBeginning();
    void whenRescanIsCalledByHandSignalThatAllShapesWereReloaded();
};

LocalShapesFinderTest::LocalShapesFinderTest()
{
}

void LocalShapesFinderTest::createFiles(unsigned int startIndex, unsigned int count, QString ext, QByteArray creator)
{
    createFilesInPath(m_dir->path(), startIndex, count, ext, creator);
}

void LocalShapesFinderTest::createFilesInPath(QString path, unsigned int startIndex, unsigned int count, QString ext, QByteArray creator)
{
    for (auto i = startIndex; i < (startIndex + count); ++i) {
        QByteArray gcodeFilename = (QString("tmpTest-%1.gcode").arg(i)).toLatin1();
        QByteArray svgFilename = (QString("tmpTest-%1.svg").arg(i)).toLatin1();
        QByteArray content = R"(
{
  "version": 1,
  "svgFilename": ")" + svgFilename + R"(",
  "name": "sandman",
  "square": true,
  "machineType": "PolyShaperOranje",
  "drawToolpath": true,
  "margin": 10.0,
  "generatedBy": ")" + creator + R"(",
  "creationTime": "2018-07-26T22:56:56.931242",
  "flatness": 0.001,
  "workpieceDimX": 400.0,
  "workpieceDimY": 450.0,
  "autoClosePath": true,
  "duration": 81,
  "pointsInsideWorkpiece": true,
  "speed": 1000.0,
  "gcodeFilename": ")" + gcodeFilename + R"("
})";
        QFile file(path + QString("/tmpTest-%1.%2").arg(i).arg(ext));
        if (!file.open(QIODevice::WriteOnly)) {
            throw QString("CANNOT CREATE psj TEMPORARY FILE!!!");
        }
        file.write(content);

        // Also create gcode and svg files
        QFile gcodeFile(path + "/" + gcodeFilename);
        if (!gcodeFile.open(QIODevice::WriteOnly)) {
            throw QString("CANNOT CREATE gcode TEMPORARY FILE!!!");
        }

        QFile svgFile(path + "/" + svgFilename);
        if (!svgFile.open(QIODevice::WriteOnly)) {
            throw QString("CANNOT CREATE svg TEMPORARY FILE!!!");
        }
    }

    QThread::msleep(300); // Small delay needed on mac
}

void LocalShapesFinderTest::init()
{
    m_dir = std::make_unique<QTemporaryDir>();
    QVERIFY(m_dir->isValid());
    m_curPath = QDir(m_dir->path()).canonicalPath();
}

void LocalShapesFinderTest::cleanup()
{
    m_dir.reset();
}

void LocalShapesFinderTest::testQFileSysteWatcherUsage()
{
    // A test to check how FileSystemWatcher works
    QFileSystemWatcher watcher;

    QSignalSpy spy(&watcher, &QFileSystemWatcher::directoryChanged);

    watcher.addPath(m_curPath);

    // No signal at start
    QVERIFY(!spy.wait(500));

    // Create a file
    QFile file(m_curPath + "/testFile");
    QVERIFY(file.open(QIODevice::WriteOnly));
    QVERIFY(spy.wait(1000));
    QCOMPARE(spy.at(0).at(0).toString(), m_curPath);

    // Remove a file
    QVERIFY(file.remove());
    QVERIFY(spy.wait(1000));
    QCOMPARE(spy.at(1).at(0).toString(), m_curPath);

    // Create a subdirectory
    QString testDir = "testDir";
    QDir curDir(m_curPath);
    QVERIFY(curDir.mkdir(testDir));
    QVERIFY(spy.wait(1000));
    QCOMPARE(spy.at(2).at(0).toString(), m_curPath);

    // Remove a subdirectory
    QVERIFY(curDir.rmdir(testDir));
    QVERIFY(spy.wait(1000));
    QCOMPARE(spy.at(3).at(0).toString(), m_curPath);

    // Remove watched directory - not working on mac
    QVERIFY(curDir.rmdir(m_curPath));
    if (QSysInfo::productType().toLower() != "osx") {
        QVERIFY(spy.wait(1000));
        QCOMPARE(spy.at(4).at(0).toString(), m_curPath);
    }

    // Recreate watched directory, no signal
    QThread::sleep(1); // Needed to have test pass on windows (mkdir might fail otherwise)
    QVERIFY(curDir.mkdir(m_curPath));
    QVERIFY(!spy.wait(1000));
}

void LocalShapesFinderTest::testCreatePsjFiles()
{
    createFiles(5, 3, "dummy");

    // Just test creation, content is tested implicitly in other tests
    QVERIFY(QFile::exists(m_curPath + "/tmpTest-5.dummy"));
    QVERIFY(QFile::exists(m_curPath + "/tmpTest-6.dummy"));
    QVERIFY(QFile::exists(m_curPath + "/tmpTest-7.dummy"));
    QVERIFY(QFile::exists(m_curPath + "/tmpTest-5.gcode"));
    QVERIFY(QFile::exists(m_curPath + "/tmpTest-6.gcode"));
    QVERIFY(QFile::exists(m_curPath + "/tmpTest-7.gcode"));
    QVERIFY(QFile::exists(m_curPath + "/tmpTest-5.svg"));
    QVERIFY(QFile::exists(m_curPath + "/tmpTest-6.svg"));
    QVERIFY(QFile::exists(m_curPath + "/tmpTest-7.svg"));
}

void LocalShapesFinderTest::loadFilesFromDirectory()
{
    LocalShapesFinder finder(m_curPath);

    createFiles(0, 3);

    // This is needed to process events from QFileWatcher
    QCoreApplication::processEvents();

    QCOMPARE(finder.shapes().size(), 3);
    QCOMPARE(finder.shapes()[m_curPath + "/tmpTest-0.psj"].svgFilename(), "tmpTest-0.svg");
    QCOMPARE(finder.shapes()[m_curPath + "/tmpTest-0.psj"].gcodeFilename(), "tmpTest-0.gcode");
    QCOMPARE(finder.shapes()[m_curPath + "/tmpTest-1.psj"].svgFilename(), "tmpTest-1.svg");
    QCOMPARE(finder.shapes()[m_curPath + "/tmpTest-1.psj"].gcodeFilename(), "tmpTest-1.gcode");
    QCOMPARE(finder.shapes()[m_curPath + "/tmpTest-2.psj"].svgFilename(), "tmpTest-2.svg");
    QCOMPARE(finder.shapes()[m_curPath + "/tmpTest-2.psj"].gcodeFilename(), "tmpTest-2.gcode");
}

void LocalShapesFinderTest::onlyLoadFilesWithPsjExtension()
{
    LocalShapesFinder finder(m_curPath);

    createFiles(0, 3);
    createFiles(6, 2, "dummy"); // These are not loaded

    // This is needed to process events from QFileWatcher
    QCoreApplication::processEvents();

    QCOMPARE(finder.shapes().size(), 3);
    QVERIFY(finder.shapes().contains(m_curPath + "/tmpTest-0.psj"));
    QVERIFY(finder.shapes().contains(m_curPath + "/tmpTest-1.psj"));
    QVERIFY(finder.shapes().contains(m_curPath + "/tmpTest-2.psj"));
}

void LocalShapesFinderTest::skipInvalidPsjFiles()
{
    LocalShapesFinder finder(m_curPath);

    createFiles(0, 3);

    // Create another invalid (empty) psj file
    QFile gcodeFile(m_curPath + QString("/tmpTest-100.psj"));
    QVERIFY(gcodeFile.open(QIODevice::WriteOnly));

    // This is needed to process events from QFileWatcher
    QCoreApplication::processEvents();

    QCOMPARE(finder.shapes().size(), 3);
    QVERIFY(finder.shapes().contains(m_curPath + "/tmpTest-0.psj"));
    QVERIFY(finder.shapes().contains(m_curPath + "/tmpTest-1.psj"));
    QVERIFY(finder.shapes().contains(m_curPath + "/tmpTest-2.psj"));
}

void LocalShapesFinderTest::skipPsjFilesWithoutACorrespondingGCodeFile()
{
    LocalShapesFinder finder(m_curPath);

    createFiles(0, 3);

    // Remove a GCode file
    QVERIFY(QFile::remove(m_curPath + QString("/tmpTest-1.gcode")));

    // This is needed to process events from QFileWatcher
    QCoreApplication::processEvents();

    QCOMPARE(finder.shapes().size(), 2);
    QVERIFY(finder.shapes().contains(m_curPath + "/tmpTest-0.psj"));
    QVERIFY(finder.shapes().contains(m_curPath + "/tmpTest-2.psj"));
}

void LocalShapesFinderTest::skipPsjFilesWithoutACorrespondingSvgFile()
{
    LocalShapesFinder finder(m_curPath);

    createFiles(0, 3);

    // Remove a GCode file
    QVERIFY(QFile::remove(m_curPath + QString("/tmpTest-1.svg")));

    // This is needed to process events from QFileWatcher
    QCoreApplication::processEvents();

    QCOMPARE(finder.shapes().size(), 2);
    QVERIFY(finder.shapes().contains(m_curPath + "/tmpTest-0.psj"));
    QVERIFY(finder.shapes().contains(m_curPath + "/tmpTest-2.psj"));
}

void LocalShapesFinderTest::emitSignalWhenShapesAreFirstFound()
{
    LocalShapesFinder finder(m_curPath);

    QSignalSpy spy(&finder, &LocalShapesFinder::shapesUpdated);

    createFiles(0, 3);

    // This is needed to process events from QFileWatcher
    QVERIFY(spy.wait(500));

    QSet<QString> expectedNewShapes{m_curPath + "/tmpTest-0.psj", m_curPath + "/tmpTest-1.psj", m_curPath + "/tmpTest-2.psj"};
    // Collect all new shapes (on windows we might receive multiple signals)
    QSet<QString> newShapes;
    for (auto s: spy) {
        newShapes.unite(s.at(0).value<QSet<QString>>());
        QVERIFY(s.at(1).value<QSet<QString>>().isEmpty());
    }
    QCOMPARE(newShapes, expectedNewShapes);
}

void LocalShapesFinderTest::onlyLoadNewFilesWhenDirectoryChanges()
{
    LocalShapesFinder finder(m_curPath);

    createFiles(0, 3, "psj", "first");

    // This is needed to process events from QFileWatcher
    QCoreApplication::processEvents();

    QCOMPARE(finder.shapes().size(), 3);

    // New files and rescan
    createFiles(1, 3, "psj", "another"); // 1 and 2 not reloaded, 3 added

    // This is needed to process events from QFileWatcher
    QCoreApplication::processEvents();

    QCOMPARE(finder.shapes().size(), 4);

    QCOMPARE(finder.shapes()[m_curPath + "/tmpTest-1.psj"].generatedBy(), "first");
    QCOMPARE(finder.shapes()[m_curPath + "/tmpTest-2.psj"].generatedBy(), "first");
    QVERIFY(finder.shapes().contains(m_curPath + "/tmpTest-3.psj"));
    QCOMPARE(finder.shapes()[m_curPath + "/tmpTest-3.psj"].generatedBy(), "another");
}

void LocalShapesFinderTest::emitSignalWhenNewShapesAreFound()
{
    LocalShapesFinder finder(m_curPath);

    QSignalSpy spy(&finder, &LocalShapesFinder::shapesUpdated);

    createFiles(0, 3);

    // This is needed to process events from QFileWatcher
    QVERIFY(spy.wait(500));

    QVERIFY(spy.count() > 0);
    const auto oldSignals = spy.count();

    // New files added
    createFiles(50, 2);

    // This is needed to process events from QFileWatcher
    QVERIFY(spy.wait(500));

    QSet<QString> expectedNewShapes{m_curPath + "/tmpTest-50.psj", m_curPath + "/tmpTest-51.psj"};
    // Collect all new shapes (on windows we might receive multiple signals)
    QSet<QString> newShapes;
    for (auto i = oldSignals; i < spy.count(); ++i) {
        newShapes.unite(spy.at(i).at(0).value<QSet<QString>>());
        QVERIFY(spy.at(i).at(1).value<QSet<QString>>().isEmpty());
    }
    QCOMPARE(newShapes, expectedNewShapes);
}

void LocalShapesFinderTest::removeShapesThatNoLongerExist()
{
    LocalShapesFinder finder(m_curPath);

    createFiles(0, 3);

    // This is needed to process events from QFileWatcher
    QCoreApplication::processEvents();

    QCOMPARE(finder.shapes().size(), 3);

    // Remove files and rescan
    QVERIFY(QFile::remove(m_curPath + "/tmpTest-1.psj"));

    // This is needed to process events from QFileWatcher
    QThread::sleep(1); // This is needed on windows
    QCoreApplication::processEvents();

    QCOMPARE(finder.shapes().size(), 2);
    QVERIFY(finder.shapes().contains(m_curPath + "/tmpTest-0.psj"));
    QVERIFY(finder.shapes().contains(m_curPath + "/tmpTest-2.psj"));
}

void LocalShapesFinderTest::emitSignalWhenShapesAreRemoved()
{
    LocalShapesFinder finder(m_curPath);

    QSignalSpy spy(&finder, &LocalShapesFinder::shapesUpdated);

    createFiles(0, 3);

    // This is needed to process events from QFileWatcher
    QVERIFY(spy.wait(500));

    QVERIFY(spy.count() > 0);
    const auto oldSignals = spy.count();

    // Remove files and rescan
    QVERIFY(QFile::remove(m_curPath + "/tmpTest-1.psj"));
    QVERIFY(QFile::remove(m_curPath + "/tmpTest-2.psj"));

    // This is needed to process events from QFileWatcher
    QVERIFY(spy.wait(500));

    QSet<QString> expectedRemovedShapes{m_curPath + "/tmpTest-1.psj", m_curPath + "/tmpTest-2.psj"};
    // Collect all removed shapes (on windows we might receive multiple signals)
    QSet<QString> removedShapes;
    for (auto i = oldSignals; i < spy.count(); ++i) {
        QVERIFY(spy.at(i).at(0).value<QSet<QString>>().isEmpty());
        removedShapes.unite(spy.at(i).at(1).value<QSet<QString>>());
    }
    QCOMPARE(removedShapes, expectedRemovedShapes);
}

void LocalShapesFinderTest::removeAllShapesIfDirectoryIsRemoved()
{
    LocalShapesFinder finder(m_curPath);

    QSignalSpy spy(&finder, &LocalShapesFinder::shapesUpdated);

    createFiles(0, 3);

    // This is needed to process events from QFileWatcher
    QVERIFY(spy.wait(500));

    QVERIFY(spy.count() > 0);
    const auto oldSignals = spy.count();

    // Remove directory (save path to use it later)
    m_dir.reset();

    // This is needed to process events from QFileWatcher
    QVERIFY(spy.wait(1000));

    QSet<QString> expectedRemovedShapes{m_curPath + "/tmpTest-0.psj", m_curPath + "/tmpTest-1.psj", m_curPath + "/tmpTest-2.psj"};
    // Collect all removed shapes (on windows we might receive multiple signals)
    QSet<QString> removedShapes;
    for (auto i = oldSignals; i < spy.count(); ++i) {
        QVERIFY(spy.at(i).at(0).value<QSet<QString>>().isEmpty());
        removedShapes.unite(spy.at(i).at(1).value<QSet<QString>>());
    }
    QCOMPARE(removedShapes, expectedRemovedShapes);

    QCOMPARE(finder.shapes().size(), 0);
}

void LocalShapesFinderTest::doNotEmitSignalIfDirectoryRemovedAndThereIsNoShape()
{
    LocalShapesFinder finder(m_curPath);

    QSignalSpy spy(&finder, &LocalShapesFinder::shapesUpdated);

    // Discard initial signals we might receive (especially on Windows)
    QThread::sleep(1);
    QCoreApplication::processEvents();

    m_dir.reset();

    // No signal emitted
    QVERIFY(!spy.wait(500));
}

void LocalShapesFinderTest::loadFilesFromDirAtStart()
{
    createFiles(0, 3);

    LocalShapesFinder finder(m_curPath);

    QCOMPARE(finder.shapes().size(), 3);

    QVERIFY(finder.shapes().contains(m_curPath + "/tmpTest-0.psj"));
    QVERIFY(finder.shapes().contains(m_curPath + "/tmpTest-1.psj"));
    QVERIFY(finder.shapes().contains(m_curPath + "/tmpTest-2.psj"));
}

void LocalShapesFinderTest::doNotEmitSignalsIfThereIsNoChange()
{
    // Here we add and remove a fiel, we should receive no signal (on Windows a
    // notification might arrive, we must discard it)

    LocalShapesFinder finder(m_curPath);

    QSignalSpy spy(&finder, &LocalShapesFinder::shapesUpdated);

    createFiles(0, 3);

    // This is needed to process events from QFileWatcher
    QVERIFY(spy.wait(500));

    // Create and remove a file
    createFiles(5, 1);
    QFile::remove(m_curPath + "/tmpTest-5.psj");

    // There must be no notification
    QVERIFY(!spy.wait(500));
}

void LocalShapesFinderTest::createDirectoryIfNotExistingAtStart()
{
    // Save path and remove dir
    const auto path = m_curPath;
    m_dir.reset();

    LocalShapesFinder finder(path);

    QSignalSpy spy(&finder, &LocalShapesFinder::shapesUpdated);

    // Path is created
    QDir dir(path);
    QVERIFY(dir.exists());

    // Discarding initial notifications
    spy.wait(500);
    const auto oldSignals = spy.count();

    // Also check we get signals on directory change
    createFilesInPath(path, 0, 1);

    QVERIFY(spy.wait(500));

    QSet<QString> expectedNewShapes{path + "/tmpTest-0.psj"};
    // Collect all new shapes (on windows we might receive multiple signals)
    QSet<QString> newShapes;
    for (auto i = oldSignals; i < spy.count(); ++i) {
        newShapes.unite(spy.at(i).at(0).value<QSet<QString>>());
        QVERIFY(spy.at(i).at(1).value<QSet<QString>>().isEmpty());
    }
    QCOMPARE(newShapes, expectedNewShapes);

    // Remove everything
    dir.removeRecursively();
}

void LocalShapesFinderTest::whenRescanIsCalledByHandReloadEverythingFromTheBeginning()
{
    // When a reload is triggered by QFileSystemWatcher we don't reload files that we already know
    // (even if they might have changed). If a reload is forced, we reload everything from scratch

    createFiles(0, 3, "psj", "UNO");

    LocalShapesFinder finder(m_curPath);

    QCOMPARE(finder.shapes().size(), 3);

    QCOMPARE(finder.shapes()[m_curPath + "/tmpTest-0.psj"].generatedBy(), "UNO");
    QCOMPARE(finder.shapes()[m_curPath + "/tmpTest-1.psj"].generatedBy(), "UNO");
    QCOMPARE(finder.shapes()[m_curPath + "/tmpTest-2.psj"].generatedBy(), "UNO");

    // Create again, they are not reloaded
    createFiles(0, 3, "psj", "DUE");

    QCOMPARE(finder.shapes()[m_curPath + "/tmpTest-0.psj"].generatedBy(), "UNO");
    QCOMPARE(finder.shapes()[m_curPath + "/tmpTest-1.psj"].generatedBy(), "UNO");
    QCOMPARE(finder.shapes()[m_curPath + "/tmpTest-2.psj"].generatedBy(), "UNO");

    // Now force reload, everything is reloaded
    finder.reload();

    QCOMPARE(finder.shapes()[m_curPath + "/tmpTest-0.psj"].generatedBy(), "DUE");
    QCOMPARE(finder.shapes()[m_curPath + "/tmpTest-1.psj"].generatedBy(), "DUE");
    QCOMPARE(finder.shapes()[m_curPath + "/tmpTest-2.psj"].generatedBy(), "DUE");
}

void LocalShapesFinderTest::whenRescanIsCalledByHandSignalThatAllShapesWereReloaded()
{
    // When a reload is triggered by QFileSystemWatcher we don't reload files that we already know
    // (even if they might have changed). If a reload is forced, we reload everything from scratch

    createFiles(0, 3, "psj", "UNO");

    LocalShapesFinder finder(m_curPath);

    QSignalSpy spy(&finder, &LocalShapesFinder::shapesUpdated);

    // Create again and force reload
    createFiles(0, 3, "psj", "DUE");
    finder.reload();

    QCOMPARE(spy.count(), 1);
    const auto& newShapes = spy.at(0).at(0).value<QSet<QString>>();
    const auto& missingShapes = spy.at(0).at(1).value<QSet<QString>>();

    const auto expectedSet = QSet<QString>{
            m_curPath + "/tmpTest-0.psj",
            m_curPath + "/tmpTest-1.psj",
            m_curPath + "/tmpTest-2.psj"
    };

    QCOMPARE(newShapes, expectedSet);
    QCOMPARE(missingShapes, expectedSet);
}

QTEST_GUILESS_MAIN(LocalShapesFinderTest)

#include "localshapesfinder_test.moc"

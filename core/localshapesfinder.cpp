#include "localshapesfinder.h"
#include <QDir>
#include <QFile>

LocalShapesFinder::LocalShapesFinder(QString path)
    : m_path(path)
{
    // Creating directory if it doesn't exist
    QDir::root().mkpath(m_path);

    // Adding now to make sure it has been created
    m_watcher.addPath(m_path);

    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, this, &LocalShapesFinder::directoryChanged);

    // Load initial dir content
    reload();
}

const QMap<QString, ShapeInfo>& LocalShapesFinder::shapes() const
{
    return m_shapes;
}

void LocalShapesFinder::reload()
{
    QDir dir(m_path);

    if (dirRemoved(dir)) {
        return;
    }

    const auto initialShapes = m_shapes.keys().toSet();

    m_shapes.clear(); // This is needed so that loadNewShapes loads all shapes
    m_shapes = loadNewShapes(listAllShapesInDir(dir));

    if (!initialShapes.isEmpty() || !m_shapes.isEmpty()) {
        emit shapesUpdated(m_shapes.keys().toSet(), initialShapes);
    }
}

void LocalShapesFinder::directoryChanged()
{
    QDir dir(m_path);

    if (dirRemoved(dir)) {
        return;
    }

    const auto initialShapes = m_shapes.keys().toSet();
    const auto currentShapes = listAllShapesInDir(dir);

    const auto newShapes = loadNewShapes(currentShapes);
    m_shapes.unite(newShapes);

    const auto missingShapes = initialShapes - currentShapes;
    for (const auto& toRemove: missingShapes) {
        m_shapes.remove(toRemove);
    }

    if (!newShapes.isEmpty() || !missingShapes.isEmpty()) {
        emit shapesUpdated(m_shapes.keys().toSet() - initialShapes, missingShapes);
    }
}

QSet<QString> LocalShapesFinder::listAllShapesInDir(QDir dir) const
{
    QSet<QString> allShapes;

    for (const auto& info: dir.entryInfoList(QStringList() << "*.psj", QDir::Files)) {
        allShapes.insert(info.canonicalFilePath());
    }

    return allShapes;
}

QMap<QString, ShapeInfo> LocalShapesFinder::loadNewShapes(QSet<QString> currentShapes) const
{
    QMap<QString, ShapeInfo> newValidShapes;

    for (const auto& shapeFile: currentShapes) {
        if (!m_shapes.contains(shapeFile)) {
            auto shapeInfo = ShapeInfo::createFromFile(shapeFile);

            if (!validShape(shapeInfo)) {
                continue;
            }

            newValidShapes[shapeFile] = shapeInfo;
        }
    }

    return newValidShapes;
}

bool LocalShapesFinder::dirRemoved(QDir& dir)
{
    if (!dir.exists()) {
        if (!m_shapes.isEmpty()) {
            emit shapesUpdated(QSet<QString>(), m_shapes.keys().toSet());
        }
        m_shapes.clear();
        return true;
    }

    return false;
}

bool LocalShapesFinder::validShape(ShapeInfo& info) const
{
    return info.isValid() &&
           QFile::exists(info.path() + "/" + info.gcodeFilename()) &&
           QFile::exists(info.path() + "/" + info.svgFilename());
}

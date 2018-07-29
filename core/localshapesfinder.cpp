#include "localshapesfinder.h"
#include <QFile>

LocalShapesFinder::LocalShapesFinder(QString path)
    : m_watcher(QStringList{path})
{
    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, this, &LocalShapesFinder::rescanDirectory);

    // Load initial dir content
    rescanDirectory(path);
}

const QMap<QString, ShapeInfo>& LocalShapesFinder::shapes() const
{
    return m_shapes;
}

void LocalShapesFinder::rescanDirectory(QString path)
{
    QDir dir(path);

    if (dirRemoved(dir)) {
        return;
    }

    QSet<QString> newShapes;
    QSet<QString> missingShapes(m_shapes.keys().toSet());
    for (const auto& info: dir.entryInfoList(QStringList() << "*.psj", QDir::Files)) {
        const auto& shapeFile = info.canonicalFilePath();
        auto shapeInfo = ShapeInfo::createFromFile(shapeFile);

        if (!validShape(shapeInfo)) {
            continue;
        }

        if (!m_shapes.contains(shapeFile)) {
            newShapes.insert(shapeFile);
            m_shapes[shapeFile] = shapeInfo;
        }

        missingShapes.remove(shapeFile);
    }

    for (auto toRemove: missingShapes) {
        m_shapes.remove(toRemove);
    }

    emit shapesUpdated(newShapes, missingShapes);
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

bool LocalShapesFinder::validShape(ShapeInfo& info)
{
    return info.isValid() && QFile::exists(info.gcodeFilename()) && QFile::exists(info.svgFilename());
}

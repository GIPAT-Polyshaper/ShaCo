#ifndef LOCALSHAPESFINDER_H
#define LOCALSHAPESFINDER_H

#include <QDir>
#include <QFileSystemWatcher>
#include <QMap>
#include <QObject>
#include <QSet>
#include "shapeinfo.h"

class LocalShapesFinder : public QObject
{
    Q_OBJECT

public:
    // path must be absolute!!!
    explicit LocalShapesFinder(QString path);

    const QMap<QString, ShapeInfo>& shapes() const;

    void reload();

private slots:
    void directoryChanged();

signals:
    // Note: newShapes and removedShapes might be partially overlapping if reload() is called
    void shapesUpdated(QSet<QString> newShapes, QSet<QString> removedShapes);

private:
    QSet<QString> listAllShapesInDir(QDir dir) const;
    QMap<QString, ShapeInfo> loadNewShapes(QSet<QString> currentShapes) const;
    bool dirRemoved(QDir& dir);
    bool validShape(ShapeInfo& info) const;

    const QString m_path;
    QFileSystemWatcher m_watcher;
    QMap<QString, ShapeInfo> m_shapes;
};

#endif // LOCALSHAPESFINDER_H

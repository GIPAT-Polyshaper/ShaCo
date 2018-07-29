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
    explicit LocalShapesFinder(QString path);

    const QMap<QString, ShapeInfo>& shapes() const;

private slots:
    void rescanDirectory(QString path);

signals:
    void shapesUpdated(QSet<QString> newShapes, QSet<QString> removedShapes);

private:
    bool dirRemoved(QDir& dir);
    bool validShape(ShapeInfo& info);

    QFileSystemWatcher m_watcher;
    QMap<QString, ShapeInfo> m_shapes;
};

#endif // LOCALSHAPESFINDER_H

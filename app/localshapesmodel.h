#ifndef LOCALSHAPESMODEL_H
#define LOCALSHAPESMODEL_H

#include <QAbstractListModel>
#include "core/localshapesfinder.h"

// TODO-TOMMY This is not tested, see comment in controller.h
// The model of local shapes
class LocalShapesModel : public QAbstractListModel
{
    Q_OBJECT

private:
    enum Roles {
        name = Qt::UserRole,
        svgFilename,
        square,
        machineType,
        drawToolpath,
        margin,
        generatedBy,
        creationTime,
        flatness,
        workpieceDimX,
        workpieceDimY,
        autoClosePath,
        duration,
        pointsInsideWorkpiece,
        speed,
        gcodeFilename
    };

public:
    enum class SortCriterion {
        Newest,
        AZ,
        ZA
    };

public:
    explicit LocalShapesModel(LocalShapesFinder& finder);

    LocalShapesModel(LocalShapesModel&) = delete;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void sortShapes(SortCriterion s);

public slots:
    void shapesUpdated(QSet<QString> newShapes, QSet<QString> removedShapes);

private:
    LocalShapesFinder& m_finder;
    QStringList m_sortedShapes;
    SortCriterion m_curSortCriterion;
};

#endif // LOCALSHAPESMODEL_H

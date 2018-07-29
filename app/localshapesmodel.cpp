#include "localshapesmodel.h"
#include <algorithm>

LocalShapesModel::LocalShapesModel(LocalShapesFinder &finder)
    : m_finder(finder)
    , m_sortedShapes(m_finder.shapes().keys())
    , m_curSortCriterion(SortCriterion::Newest)
{
    connect(&m_finder, &LocalShapesFinder::shapesUpdated, this, &LocalShapesModel::shapesUpdated);

    sortShapes(m_curSortCriterion);
}

int LocalShapesModel::rowCount(const QModelIndex &) const
{
    return m_sortedShapes.size();
}

QVariant LocalShapesModel::data(const QModelIndex &index, int role) const
{
    if (index.row() < 0 || index.row() > m_sortedShapes.size() ||
            !m_finder.shapes().contains(m_sortedShapes[index.row()])) {
        return QVariant();
    }

    const auto& info = m_finder.shapes()[m_sortedShapes[index.row()]];

    switch (role) {
        case name:                  return info.name();
        case svgFilename:           return info.svgFilename();
        case square:                return info.square();
        case machineType:           return info.machineType();
        case drawToolpath:          return info.drawToolpath();
        case margin:                return info.margin();
        case generatedBy:           return info.generatedBy();
        case creationTime:          return info.creationTime();
        case flatness:              return info.flatness();
        case workpieceDimX:         return info.workpieceDimX();
        case workpieceDimY:         return info.workpieceDimY();
        case autoClosePath:         return info.autoClosePath();
        case duration:              return info.duration();
        case pointsInsideWorkpiece: return info.pointsInsideWorkpiece();
        case speed:                 return info.speed();
        case gcodeFilename:         return info.gcodeFilename();
        default:                    return QVariant();
    }
}

QHash<int, QByteArray> LocalShapesModel::roleNames() const
{
    QHash<int, QByteArray> roles;

    roles[name] = "name";
    roles[svgFilename] = "svgFilename";
    roles[square] = "square";
    roles[machineType] = "machineType";
    roles[drawToolpath] = "drawToolpath";
    roles[margin] = "margin";
    roles[generatedBy] = "generatedBy";
    roles[creationTime] = "creationTime";
    roles[flatness] = "flatness";
    roles[workpieceDimX] = "workpieceDimX";
    roles[workpieceDimY] = "workpieceDimY";
    roles[autoClosePath] = "autoClosePath";
    roles[duration] = "duration";
    roles[pointsInsideWorkpiece] = "pointsInsideWorkpiece";
    roles[speed] = "speed";
    roles[gcodeFilename] = "gcodeFilename";

    return roles;
}

void LocalShapesModel::sortShapes(SortCriterion s)
{
    m_curSortCriterion = s;

    beginResetModel();

    switch (s) {
        case SortCriterion::Newest:
            std::sort(m_sortedShapes.begin(), m_sortedShapes.end(),
                      [this](const QString& n1, const QString& n2) {
                          return m_finder.shapes()[n1].creationTime() > m_finder.shapes()[n2].creationTime();
                      });
            break;
        case SortCriterion::AZ:
            std::sort(m_sortedShapes.begin(), m_sortedShapes.end(), [](const QString& n1, const QString& n2) {
                          return n1 < n2;
                      });
            break;
        case SortCriterion::ZA:
            std::sort(m_sortedShapes.begin(), m_sortedShapes.end(), [](const QString& n1, const QString& n2) {
                          return n1 > n2;
                      });
            break;
    }

    endResetModel();
}

void LocalShapesModel::shapesUpdated(QSet<QString>, QSet<QString>)
{
    m_sortedShapes = m_finder.shapes().keys();
    sortShapes(m_curSortCriterion);
}

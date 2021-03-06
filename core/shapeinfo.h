#ifndef SHAPEINFO_H
#define SHAPEINFO_H

#include <QDateTime>
#include <QString>

class ShapeInfo
{
public:
    static ShapeInfo createFromFile(QString filename);

public:
    ShapeInfo();

private:
    ShapeInfo(unsigned int version, QString path, QString psjFilename, QString name,
              QString svgFilename, bool square, QString machineType, bool drawToolpath,
              double margin, QString generatedBy, QDateTime creationTime, double flatness,
              double workpieceDimX, double workpieceDimY, bool autoClosePath,
              unsigned int duration, bool pointsInsideWorkpiece, double speed,
              QString gcodeFilename);

public:
    bool isValid() const
    {
        return m_isValid;
    }

    unsigned int version() const
    {
        return m_version;
    }

    QString path() const // The canonical file path containing the loaded file
    {
        return m_path;
    }

    QString psjFilename() const
    {
        return m_psjFilename;
    }

    QString name() const
    {
        return m_name;
    }

    QString svgFilename() const
    {
        return m_svgFilename;
    }

    bool square() const
    {
        return m_square;
    }

    QString machineType() const
    {
        return m_machineType;
    }

    bool drawToolpath() const
    {
        return m_drawToolpath;
    }

    double margin() const // in mm
    {
        return m_margin;
    }

    QString generatedBy() const
    {
        return m_generatedBy;
    }

    QDateTime creationTime() const
    {
        return m_creationTime;
    }

    double flatness() const
    {
        return m_flatness;
    }

    double workpieceDimX() const // in mm
    {
        return m_workpieceDimX;
    }

    double workpieceDimY() const // in mm
    {
        return m_workpieceDimY;
    }

    bool autoClosePath() const
    {
        return m_autoClosePath;
    }

    unsigned int duration() const // in seconds
    {
        return m_duration;
    }

    bool pointsInsideWorkpiece() const
    {
        return m_pointsInsideWorkpiece;
    }

    double speed() const // in mm/min
    {
        return m_speed;
    }

    QString gcodeFilename() const
    {
        return m_gcodeFilename;
    }

private:
    bool m_isValid;
    unsigned int m_version;
    QString m_path;
    QString m_psjFilename;
    QString m_name;
    QString m_svgFilename;
    bool m_square;
    QString m_machineType;
    bool m_drawToolpath;
    double m_margin;
    QString m_generatedBy;
    QDateTime m_creationTime;
    double m_flatness;
    double m_workpieceDimX;
    double m_workpieceDimY;
    bool m_autoClosePath;
    unsigned int m_duration;
    bool m_pointsInsideWorkpiece;
    double m_speed;
    QString m_gcodeFilename;
};

#endif // SHAPEINFO_H

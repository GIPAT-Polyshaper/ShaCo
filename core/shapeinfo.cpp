#include "shapeinfo.h"
#include <functional>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>

namespace {
    class MissingFieldException {};
    class InvalidFieldTypeException {};

    template <class T>
    T extractField(const QJsonObject& obj, QString fieldName, bool (QJsonValue::*check_f)() const,
                   std::function<T(const QJsonValue&)> conv_f)
    {
        const auto& value = obj.value(fieldName);
        if (value.isUndefined()) {
            throw MissingFieldException();
        }
        if (!(value.*check_f)()) {
            throw InvalidFieldTypeException();
        }

        return conv_f(value);
    }
}

ShapeInfo ShapeInfo::createFromFile(QString filename)
{
    QFile file(filename);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return ShapeInfo();
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());

    if (doc.isNull()) {
        return ShapeInfo();
    }

    const auto& obj = doc.object();

    const auto version = static_cast<unsigned int>(obj.value("version").toInt(0));
    if (version != 1u) {
        return ShapeInfo();
    }

    const auto fileInfo = QFileInfo(filename);
    const auto path = fileInfo.canonicalPath();
    const auto psjFilename = fileInfo.fileName();

    try {
        return ShapeInfo(
            version,
            path,
            psjFilename,
            extractField<QString>(obj, "name", &QJsonValue::isString, [](const QJsonValue& v){return v.toString();}),
            extractField<QString>(obj, "svgFilename", &QJsonValue::isString, [](const QJsonValue& v){return v.toString();}),
            extractField<bool>(obj, "square", &QJsonValue::isBool, [](const QJsonValue& v){return v.toBool();}),
            extractField<QString>(obj, "machineType", &QJsonValue::isString, [](const QJsonValue& v){return v.toString();}),
            extractField<bool>(obj, "drawToolpath", &QJsonValue::isBool, [](const QJsonValue& v){return v.toBool();}),
            extractField<double>(obj, "margin", &QJsonValue::isDouble, [](const QJsonValue& v){return v.toDouble();}),
            extractField<QString>(obj, "generatedBy", &QJsonValue::isString, [](const QJsonValue& v){return v.toString();}),
            extractField<QDateTime>(obj, "creationTime", &QJsonValue::isString, [](const QJsonValue& v){
                auto date = QDateTime::fromString(v.toString(), Qt::ISODateWithMs);
                if (!date.isValid()) {
                    throw InvalidFieldTypeException();
                }
                return date;
            }),
            extractField<double>(obj, "flatness", &QJsonValue::isDouble, [](const QJsonValue& v){return v.toDouble();}),
            extractField<double>(obj, "workpieceDimX", &QJsonValue::isDouble, [](const QJsonValue& v){
                return v.toDouble();
            }),
            extractField<double>(obj, "workpieceDimY", &QJsonValue::isDouble, [](const QJsonValue& v){
                return v.toDouble();
            }),
            extractField<bool>(obj, "autoClosePath", &QJsonValue::isBool, [](const QJsonValue& v){return v.toBool();}),
            extractField<unsigned int>(obj, "duration", &QJsonValue::isDouble, [](const QJsonValue& v){
                auto durationInt = v.toInt(-1);
                if (durationInt < 0) {
                    throw InvalidFieldTypeException();
                }
                return static_cast<unsigned int>(durationInt);
            }),
            extractField<bool>(obj, "pointsInsideWorkpiece", &QJsonValue::isBool, [](const QJsonValue& v){
                return v.toBool();
            }),
            extractField<double>(obj, "speed", &QJsonValue::isDouble, [](const QJsonValue& v){return v.toDouble();}),
            extractField<QString>(obj, "gcodeFilename", &QJsonValue::isString, [](const QJsonValue& v){
                return v.toString();
            }));
    } catch (MissingFieldException&) {
        return ShapeInfo();
    } catch (InvalidFieldTypeException&) {
        return ShapeInfo();
    }
}

ShapeInfo::ShapeInfo()
    : m_isValid(false)
    , m_version(0)
    , m_name()
    , m_svgFilename()
    , m_square(false)
    , m_machineType()
    , m_drawToolpath(false)
    , m_margin(0.0)
    , m_generatedBy()
    , m_creationTime()
    , m_flatness(0.0)
    , m_workpieceDimX(0.0)
    , m_workpieceDimY(0.0)
    , m_autoClosePath(false)
    , m_duration(0)
    , m_pointsInsideWorkpiece(false)
    , m_speed(0.0)
    , m_gcodeFilename()
{
}

ShapeInfo::ShapeInfo(unsigned int version, QString path, QString psjFilename, QString name,
                     QString svgFilename, bool square, QString machineType, bool drawToolpath,
                     double margin, QString generatedBy, QDateTime creationTime, double flatness,
                     double workpieceDimX, double workpieceDimY, bool autoClosePath,
                     unsigned int duration, bool pointsInsideWorkpiece, double speed,
                     QString gcodeFilename)
    : m_isValid(true)
    , m_version(version)
    , m_path(path)
    , m_psjFilename(psjFilename)
    , m_name(name)
    , m_svgFilename(svgFilename)
    , m_square(square)
    , m_machineType(machineType)
    , m_drawToolpath(drawToolpath)
    , m_margin(margin)
    , m_generatedBy(generatedBy)
    , m_creationTime(creationTime)
    , m_flatness(flatness)
    , m_workpieceDimX(workpieceDimX)
    , m_workpieceDimY(workpieceDimY)
    , m_autoClosePath(autoClosePath)
    , m_duration(duration)
    , m_pointsInsideWorkpiece(pointsInsideWorkpiece)
    , m_speed(speed)
    , m_gcodeFilename(gcodeFilename)
{
}

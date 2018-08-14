#ifndef MACHINEINFO_H
#define MACHINEINFO_H

#include <memory>
#include <QMetaType>
#include <QString>

// Subclasses are meant to be shared across thread, make sure it is thread safe (e.g. only constant
// data members)
class MachineInfo {
public:
    static const bool metatypeRegistered;

    // Returns empty unique_ptr if parsing of string fails
    static std::unique_ptr<MachineInfo> createFromString(QByteArray s);

protected:
    MachineInfo(QString partNumber, QString serialNumber, QString firmwareVersion);

public:
    virtual ~MachineInfo();

    QString partNumber() const;
    QString serialNumber() const;
    QString firmwareVersion() const;

    virtual QString machineName() const = 0;
    virtual float maxWireTemperature() const = 0;

private:
    const QString m_partNumber;
    const QString m_serialNumber;
    const QString m_firmwareVersion;
};
Q_DECLARE_METATYPE(MachineInfo*)

#endif // MACHINEINFO_H

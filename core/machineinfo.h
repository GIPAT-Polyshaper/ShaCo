#ifndef MACHINEINFO_H
#define MACHINEINFO_H

#include <QMetaType>
#include <QString>

class MachineInfo {
private:
    static const bool registered;

public:
    static MachineInfo createFromString(QByteArray s);

public:
    MachineInfo();

    MachineInfo(QString machineName, QString partNumber, QString serialNumber, QString firmwareVersion);

    QString machineName() const
    {
        return m_machineName;
    }

    QString partNumber() const
    {
        return m_partNumber;
    }

    QString serialNumber() const
    {
        return m_serialNumber;
    }

    QString firmwareVersion() const
    {
        return m_firmwareVersion;
    }

    bool isValid() const
    {
        return m_isValid;
    }

private:
    const QString m_machineName;
    const QString m_partNumber;
    const QString m_serialNumber;
    const QString m_firmwareVersion;
    const bool m_isValid;
};
Q_DECLARE_METATYPE(MachineInfo)

#endif // MACHINEINFO_H

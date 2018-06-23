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

    MachineInfo(QString machineName, QString firmwareVersion);

    QString machineName() const
    {
        return m_machineName;
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
    const QString m_firmwareVersion;
    const bool m_isValid;
};
Q_DECLARE_METATYPE(MachineInfo)

#endif // MACHINEINFO_H

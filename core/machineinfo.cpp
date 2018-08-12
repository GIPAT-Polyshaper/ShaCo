#include "machineinfo.h"
#include <QRegularExpression>
#include <QRegularExpressionMatch>

namespace {
    bool registerMachineInfoMetaType()
    {
        static bool registered = false;

        if (!registered) {
            qRegisterMetaType<MachineInfo>();

            registered = true;
        }

        return registered;
    }

    QRegularExpression parseMachineInfoStr("\\[PolyShaper (.*)\\]\\[([^ ]*) ([^ ]*) ([^] ]*)\\]");
}

const bool MachineInfo::registered = registerMachineInfoMetaType();

MachineInfo MachineInfo::createFromString(QByteArray s)
{
    QRegularExpressionMatch match = parseMachineInfoStr.match(QString(s));

    if (match.hasMatch()) {
        return MachineInfo(match.captured(1), match.captured(2), match.captured(3), match.captured(4));
    } else {
        return MachineInfo();
    }
}

MachineInfo::MachineInfo()
    : m_machineName()
    , m_firmwareVersion()
    , m_isValid(false)
{
}

MachineInfo::MachineInfo(QString machineName, QString partNumber, QString serialNumber, QString firmwareVersion)
    : m_machineName(machineName)
    , m_partNumber(partNumber)
    , m_serialNumber(serialNumber)
    , m_firmwareVersion(firmwareVersion)
    , m_isValid(true)
{
}

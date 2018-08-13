#include "machineinfo.h"
#include <QRegularExpression>
#include <QRegularExpressionMatch>

namespace {
    bool registerMachineInfoMetaType()
    {
        static bool registered = false;

        if (!registered) {
            qRegisterMetaType<MachineInfo*>();
            registered = true;
        }

        return registered;
    }

    QRegularExpression parseMachineInfoStr("\\[PolyShaper (.*)\\]\\[([^ ]*) ([^ ]*) ([^] ]*)\\]");
}

const bool MachineInfo::metatypeRegistered = registerMachineInfoMetaType();

MachineInfo::MachineInfo(QString partNumber, QString serialNumber, QString firmwareVersion)
    : m_partNumber(partNumber)
    , m_serialNumber(serialNumber)
    , m_firmwareVersion(firmwareVersion)
{
}

MachineInfo::~MachineInfo()
{
}

QString MachineInfo::partNumber() const
{
    return m_partNumber;
}

QString MachineInfo::serialNumber() const
{
    return m_serialNumber;
}

QString MachineInfo::firmwareVersion() const
{
    return m_firmwareVersion;
}

class GenericMachineInfo : public MachineInfo
{
public:
    GenericMachineInfo(QString machineName, QString partNumber, QString serialNumber, QString firmwareVersion)
        : MachineInfo(partNumber, serialNumber, firmwareVersion)
        , m_machineName(machineName)
    {}

    virtual ~GenericMachineInfo() override
    {}

    virtual QString machineName() const override
    {
        return m_machineName;
    }

    virtual float maxWireTemperature() const override
    {
        return 100.0f;
    }

private:
    const QString m_machineName;
};

std::unique_ptr<MachineInfo> MachineInfo::createFromString(QByteArray s)
{
    QRegularExpressionMatch match = parseMachineInfoStr.match(QString(s));

    if (match.hasMatch()) {
        return std::make_unique<GenericMachineInfo>(match.captured(1), match.captured(2),
                                                    match.captured(3), match.captured(4));
    } else {
        return std::unique_ptr<MachineInfo>();
    }
}

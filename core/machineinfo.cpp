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

class OranjeMachineInfo : public MachineInfo
{
public:
    OranjeMachineInfo(QString partNumber, QString serialNumber, QString firmwareVersion)
        : MachineInfo(partNumber, serialNumber, firmwareVersion)
    {}

    virtual ~OranjeMachineInfo() override
    {}

    virtual QString machineName() const override
    {
        return "Oranje";
    }

    virtual float maxWireTemperature() const override
    {
        return 35.0f;
    }
};

class AzulMachineInfo : public MachineInfo
{
public:
    AzulMachineInfo(QString partNumber, QString serialNumber, QString firmwareVersion)
        : MachineInfo(partNumber, serialNumber, firmwareVersion)
    {}

    virtual ~AzulMachineInfo() override
    {}

    virtual QString machineName() const override
    {
        return "Azul";
    }

    virtual float maxWireTemperature() const override
    {
        return 75.0f;
    }
};

std::unique_ptr<MachineInfo> MachineInfo::createFromString(QByteArray s)
{
    QRegularExpressionMatch match = parseMachineInfoStr.match(QString(s));

    if (match.hasMatch()) {
        const auto& machineName = match.captured(1);

        if (machineName == "Oranje") {
            return std::make_unique<OranjeMachineInfo>(match.captured(2), match.captured(3),
                                                       match.captured(4));
        } else if (machineName == "Azul") {
            return std::make_unique<AzulMachineInfo>(match.captured(2), match.captured(3),
                                                     match.captured(4));
        } else {
            return std::make_unique<GenericMachineInfo>(machineName, match.captured(2),
                                                        match.captured(3), match.captured(4));
        }
    } else {
        return std::unique_ptr<MachineInfo>();
    }
}

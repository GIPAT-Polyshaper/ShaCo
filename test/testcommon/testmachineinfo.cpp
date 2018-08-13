#include "testmachineinfo.h"

TestMachineInfo::TestMachineInfo(QString machineName, QString partNumber, QString serialNumber,
                                 QString firmwareVersion, float maxWireTemperature)
    : MachineInfo(partNumber, serialNumber, firmwareVersion)
    , m_machineName(machineName)
    , m_maxWireTemperature(maxWireTemperature)
{}

TestMachineInfo::~TestMachineInfo()
{}

QString TestMachineInfo::machineName() const
{
    return m_machineName;
}

float TestMachineInfo::maxWireTemperature() const
{
    return m_maxWireTemperature;
}

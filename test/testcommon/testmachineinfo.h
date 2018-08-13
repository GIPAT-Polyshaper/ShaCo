#ifndef TESTMACHINEINFO_H
#define TESTMACHINEINFO_H

#include "core/machineinfo.h"

class TestMachineInfo : public MachineInfo {
public:
    TestMachineInfo(QString machineName = "a", QString partNumber = "pn", QString serialNumber = "sn",
                    QString firmwareVersion = "1", float maxWireTemperature = 100.0f);
    virtual ~TestMachineInfo() override;

    virtual QString machineName() const override;
    virtual float maxWireTemperature() const override;

private:
    const QString m_machineName;
    const float m_maxWireTemperature;
};

#endif // TESTMACHINEINFO_H

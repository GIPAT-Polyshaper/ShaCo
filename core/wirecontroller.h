#ifndef WIRECONTROLLER_H
#define WIRECONTROLLER_H

#include <QObject>
#include "machinecommunication.h"

// NOTE we should perhaps tell this class when a set-temperature command is sent, e.g. via terminal
//      or gcodesender. For the moment we ignore this, our g-codes should not have set-temperature
//      instructions and terminal is not meant to be used by normal users. To make things clear:
//      this works as long as no temperature command is issued outside this class.
class WireController : public QObject
{
    Q_OBJECT
public:
    explicit WireController(MachineCommunication* communicator);

    float temperature() const;
    bool isWireOn() const;
    float minRealTimeTemperature() const;
    float maxRealTimeTemperature() const;
    int switchWireOnCommandLength() const;
    int switchWireOffCommandLength() const;

public slots:
    void setTemperature(float temperature);
    void setRealTimeTemperature(float temperature);
    void resetRealTimeTemperature();
    void switchWireOn();
    void switchWireOff();

signals:
    void temperatureChanged(float temperature);
    void wireOn();
    void wireOff();

private slots:
    void machineInitialized();

private:
    void emitTemperatureChanged();
    void forceWireOff();

    MachineCommunication* const m_communicator;
    bool m_wireOn;
    float m_baseTemperature; // The temperature set using setTemperature()
    int m_realTimePercent;
};

#endif // WIRETEMPERATURECONTROLLER_H

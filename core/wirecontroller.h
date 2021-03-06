#ifndef WIRECONTROLLER_H
#define WIRECONTROLLER_H

#include <QObject>
#include "commandsender.h"
#include "machinecommunication.h"

// NOTE we should perhaps tell this class when a set-temperature command is sent, e.g. via terminal
//      or gcodesender. For the moment we ignore this, our g-codes should not have set-temperature
//      instructions and terminal is not meant to be used by normal users. To make things clear:
//      this works as long as no temperature command is issued outside this class.
// NOTE now that we have a class polling for machine status, we might close the loop reading the
//      base and overridden temperatura from there (leave for later)
class WireController : public QObject
{
    Q_OBJECT
public:
    explicit WireController(MachineCommunication* communicator, CommandSender* commandSender);

    float temperature() const;
    bool isWireOn() const;
    float minRealTimeTemperature() const;
    float maxRealTimeTemperature() const;

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
    CommandSender* const m_commandSender;
    bool m_wireOn;
    float m_baseTemperature; // The temperature set using setTemperature()
    int m_realTimePercent;
    float m_machineScaleFactor;
};

#endif // WIRETEMPERATURECONTROLLER_H

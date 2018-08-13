#include "wirecontroller.h"
#include <cmath>
#include "immediatecommands.h"

namespace {
    const int minPercentualRealTimeTemperature = 10;
    const int maxPercentualRealTimeTemperature = 200;

    const QByteArray wireOnCommand("M3");
    const QByteArray wireOffCommand("M5");
}

WireController::WireController(MachineCommunication *communicator, CommandSender *commandSender)
    : m_communicator(communicator)
    , m_commandSender(commandSender)
    , m_wireOn(false)
    , m_baseTemperature(0.0f)
    , m_realTimePercent(100)
{
    connect(m_communicator, &MachineCommunication::machineInitialized, this, &WireController::machineInitialized);
}

float WireController::temperature() const
{
    return m_realTimePercent * m_baseTemperature / 100.0f;
}

bool WireController::isWireOn() const
{
    return m_wireOn;
}

float WireController::minRealTimeTemperature() const
{
    return static_cast<float>(minPercentualRealTimeTemperature) * m_baseTemperature / 100.0f;
}

float WireController::maxRealTimeTemperature() const
{
    return static_cast<float>(maxPercentualRealTimeTemperature) * m_baseTemperature / 100.0f;
}

void WireController::setTemperature(float temperature)
{
    m_baseTemperature = temperature;

    // Always reset realtime override: here we want to set exactly the requested temperature
    m_communicator->writeData(QByteArray(1, ImmediateCommands::resetTemperature));
    m_realTimePercent = 100;

    int intTemp = static_cast<int>(std::round(m_baseTemperature));
    QByteArray data = "S" + QString::number(intTemp).toLatin1();
    m_commandSender->sendCommand(data);

    emitTemperatureChanged();
}

void WireController::setRealTimeTemperature(float temperature)
{
    const int curPercent = m_realTimePercent;
    m_realTimePercent = static_cast<int>(std::round(temperature / m_baseTemperature * 100.0f));
    m_realTimePercent =
        std::min(maxPercentualRealTimeTemperature, std::max(minPercentualRealTimeTemperature, m_realTimePercent));

    if (curPercent == m_realTimePercent) {
        return;
    }

    const int percentualVariations = std::abs(curPercent - m_realTimePercent);

    const int numCoarseVariations = percentualVariations / 10;
    const int numFineVariations = percentualVariations % 10;

    const char coarseCommand = (temperature < m_baseTemperature) ? ImmediateCommands::coarseTemperatureDecrement : ImmediateCommands::coarseTemperatureIncrement;
    const char fineCommand = (temperature < m_baseTemperature) ? ImmediateCommands::fineTemperatureDecrement : ImmediateCommands::fineTemperatureIncrement;
    QByteArray message(numCoarseVariations + numFineVariations, coarseCommand);
    for (auto i = numCoarseVariations; i < message.size(); ++i) {
        message[i] = fineCommand;
    }
    m_communicator->writeData(message);

    emitTemperatureChanged();
}

void WireController::resetRealTimeTemperature()
{
    if (m_realTimePercent == 100) {
        return;
    }

    m_communicator->writeData(QByteArray(1, ImmediateCommands::resetTemperature));
    m_realTimePercent = 100;

    emitTemperatureChanged();
}

void WireController::switchWireOn()
{
    if (m_wireOn) {
        return;
    }

    m_commandSender->sendCommand(wireOnCommand);
    m_wireOn = true;

    emit wireOn();
}

void WireController::switchWireOff()
{
    if (!m_wireOn) {
        return;
    }

    forceWireOff();
}

void WireController::machineInitialized()
{
    forceWireOff();
    setTemperature(temperature());
}

void WireController::emitTemperatureChanged()
{
    emit temperatureChanged(temperature());
}

void WireController::forceWireOff()
{
    m_commandSender->sendCommand(wireOffCommand);
    m_wireOn = false;

    emit wireOff();
}

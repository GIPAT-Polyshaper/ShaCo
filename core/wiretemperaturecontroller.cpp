#include "wiretemperaturecontroller.h"
#include <cmath>

namespace {
    const float initialTemperature = 30.0f;
    const int minPercentualRealTimeTemperature = 10;
    const int maxPercentualRealTimeTemperature = 200;
    const char resetCommand = 0x99;
    const char coarseIncrementCOmmand = 0x9A;
    const char coarseDecrementCommand = 0x9B;
    const char fineIncrementCommand = 0x9C;
    const char fineDecrementCommand = 0x9D;
}

WireTemperatureController::WireTemperatureController(MachineCommunication *communicator)
    : m_communicator(communicator)
    , m_wireOn(false)
    , m_baseTemperature(initialTemperature)
    , m_realTimePercent(100)
{
    connect(m_communicator, &MachineCommunication::portOpened, this, &WireTemperatureController::portOpened);
}

float WireTemperatureController::temperature() const
{
    return m_realTimePercent * m_baseTemperature / 100.0f;
}

bool WireTemperatureController::isWireOn() const
{
    return m_wireOn;
}

float WireTemperatureController::minRealTimeTemperature() const
{
    return static_cast<float>(minPercentualRealTimeTemperature) * m_baseTemperature / 100.0f;
}

float WireTemperatureController::maxRealTimeTemperature() const
{
    return static_cast<float>(maxPercentualRealTimeTemperature) * m_baseTemperature / 100.0f;
}

void WireTemperatureController::setTemperature(float temperature)
{
    m_baseTemperature = temperature;

    int intTemp = static_cast<int>(std::round(m_baseTemperature));
    QByteArray data = "XS" + QString::number(intTemp).toLatin1(); // X is replaced with reset command
    // Always reset realtime override: here we want to set exactly the requested temperature
    data[0] = resetCommand;
    m_communicator->writeLine(data);
    m_realTimePercent = 100;

    emitTemperatureChanged();
}

void WireTemperatureController::setRealTimeTemperature(float temperature)
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

    const char coarseCommand = (temperature < m_baseTemperature) ? coarseDecrementCommand : coarseIncrementCOmmand;
    const char fineCommand = (temperature < m_baseTemperature) ? fineDecrementCommand : fineIncrementCommand;
    QByteArray message(numCoarseVariations + numFineVariations, coarseCommand);
    for (auto i = numCoarseVariations; i < message.size(); ++i) {
        message[i] = fineCommand;
    }
    m_communicator->writeData(message);

    emitTemperatureChanged();
}

void WireTemperatureController::resetRealTimeTemperature()
{
    if (m_realTimePercent == 100) {
        return;
    }

    m_communicator->writeData(QByteArray(1, resetCommand));
    m_realTimePercent = 100;

    emitTemperatureChanged();
}

void WireTemperatureController::switchWireOn()
{
    if (m_wireOn) {
        return;
    }

    m_communicator->writeLine("M3");
    m_wireOn = true;

    emit wireOn();
}

void WireTemperatureController::switchWireOff()
{
    if (!m_wireOn) {
        return;
    }

    m_communicator->writeLine("M5");
    m_wireOn = false;

    emit wireOff();
}

void WireTemperatureController::portOpened()
{
    setTemperature(initialTemperature);
}

void WireTemperatureController::emitTemperatureChanged()
{
    emit temperatureChanged(temperature());
}

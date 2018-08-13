#include "settings.h"

namespace {
    const char* characterSendDelayUs_pname = "characterSendDelayUs";
    constexpr unsigned long characterSendDelayUs_default = 0;

    const char* wireTemperature_pname = "wireTemperature";
    constexpr float wireTemperature_default = 30.0f;
}

Settings::Settings()
{
}

unsigned long Settings::characterSendDelayUs() const
{
    bool ok;
    const auto v = m_settings.value(characterSendDelayUs_pname).toUInt(&ok);

    return ok ? v : characterSendDelayUs_default;
}

void Settings::setCharacterSendDelayUs(unsigned long us)
{
    m_settings.setValue(characterSendDelayUs_pname, QVariant(static_cast<qulonglong>(us)));
}

float Settings::wireTemperature() const
{
    bool ok;
    const auto v = m_settings.value(wireTemperature_pname).toFloat(&ok);

    return ok ? v : wireTemperature_default;
}

void Settings::setWireTemperature(float t)
{
    m_settings.setValue(wireTemperature_pname, t);
}

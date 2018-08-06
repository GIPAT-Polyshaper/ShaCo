#include "settings.h"

namespace {
    const char* characterSendDelayUs_pname = "characterSendDelayUs";
    constexpr unsigned long characterSendDelayUs_default = 0;
}

Settings::Settings()
{
}

unsigned long Settings::characterSendDelayUs() const
{
    bool ok;
    const unsigned int v = m_settings.value(characterSendDelayUs_pname).toUInt(&ok);

    return ok ? v : characterSendDelayUs_default;
}

void Settings::setCharacterSendDelayUs(unsigned long us)
{
    m_settings.setValue(characterSendDelayUs_pname, QVariant(static_cast<qulonglong>(us)));
}

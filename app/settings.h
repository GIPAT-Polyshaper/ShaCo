#ifndef SETTINGS_H
#define SETTINGS_H

#include <QSettings>

class Settings {
public:
    Settings();

    unsigned long characterSendDelayUs() const;
    void setCharacterSendDelayUs(unsigned long us);

private:
    QSettings m_settings;
};

#endif // SETTINGS_H

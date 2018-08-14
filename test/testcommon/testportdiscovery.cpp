#include "testportdiscovery.h"
#include <QString>

TestPortDiscovery::TestPortDiscovery(SerialPortInterface* serialPort)
    : m_serialPort(serialPort)
{
}

std::unique_ptr<SerialPortInterface> TestPortDiscovery::obtainPort()
{
    emit serialPortMoved();

    return std::move(m_serialPort);
}

void TestPortDiscovery::setCharacterSendDelayUs(unsigned long)
{
    throw QString("TestPortDiscovery::setCharacterSendDelayUs should not be used in this test");
}

void TestPortDiscovery::start()
{
}

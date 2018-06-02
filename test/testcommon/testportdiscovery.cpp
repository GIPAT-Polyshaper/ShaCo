#include "testportdiscovery.h"

TestPortDiscovery::TestPortDiscovery(SerialPortInterface* serialPort)
    : m_serialPort(serialPort)
{
}

std::unique_ptr<SerialPortInterface> TestPortDiscovery::obtainPort()
{
    emit serialPortMoved();

    return std::move(m_serialPort);
}

void TestPortDiscovery::start()
{
}

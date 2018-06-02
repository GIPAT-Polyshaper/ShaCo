#ifndef TESTPORTDISCOVERY_H
#define TESTPORTDISCOVERY_H

#include "core/portdiscovery.h"
#include "core/serialport.h"

class TestPortDiscovery : public AbstractPortDiscovery {
    Q_OBJECT

public:
    TestPortDiscovery(SerialPortInterface* serialPort);

    std::unique_ptr<SerialPortInterface> obtainPort() override;
    void start() override;

signals:
    void serialPortMoved();

private:
    std::unique_ptr<SerialPortInterface> m_serialPort;
};

#endif // TESTPORTDISCOVERY_H

#include "utils.h"
#include "testportdiscovery.h"

std::pair<std::unique_ptr<MachineCommunication>, TestSerialPort*> createCommunicator(int hardResetDelay)
{
    auto serialPort = new TestSerialPort();
    TestPortDiscovery portDiscoverer(serialPort);
    auto communicator = std::make_unique<MachineCommunication>(hardResetDelay);
    communicator->portFound(MachineInfo("a", "1"), &portDiscoverer);

    return std::move(std::make_pair(std::move(communicator), serialPort));
}

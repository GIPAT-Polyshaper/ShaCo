#include "utils.h"
#include <memory>
#include "core/machinecommunication.h"
#include "core/machineinfo.h"
#include "testportdiscovery.h"

std::pair<std::unique_ptr<MachineCommunication>, TestSerialPort*> createCommunicator(MachineInfo* info, int hardResetDelay)
{
    auto serialPort = new TestSerialPort();
    TestPortDiscovery portDiscoverer(serialPort);
    auto communicator = std::make_unique<MachineCommunication>(hardResetDelay);
    communicator->portFound(info, &portDiscoverer);

    return std::make_pair(std::move(communicator), serialPort);
}

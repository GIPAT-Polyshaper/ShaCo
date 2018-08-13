#ifndef UTILS_H
#define UTILS_H

#include <utility>
#include <memory>
#include "core/machinecommunication.h"
#include "testserialport.h"

std::pair<std::unique_ptr<MachineCommunication>, TestSerialPort*> createCommunicator(MachineInfo* info,
                                                                                     int hardResetDelay = 100);

#endif // UTILS_H

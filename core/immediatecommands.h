#ifndef IMMEDIATECOMMANDS_H
#define IMMEDIATECOMMANDS_H

namespace ImmediateCommands {
    const char feedHold = '!';
    const char resumeFeedHold = '~';
    const char softReset = 0x18;
    const char hardReset = 0xC0;
    const char resetTemperature = 0x99;
    const char coarseTemperatureIncrement= 0x9A;
    const char coarseTemperatureDecrement = 0x9B;
    const char fineTemperatureIncrement = 0x9C;
    const char fineTemperatureDecrement = 0x9D;
    const char statusReportQuery = '?';
}

#endif // IMMEDIATECOMMANDS_H

#ifndef MACHINECOMMUNICATION_H
#define MACHINECOMMUNICATION_H

#include <memory>
#include <QObject>
#include "portdiscovery.h"
#include "machineinfo.h"
#include "serialport.h"

// TODO-TOMMY It might be useful to add a function to obtain the machien status (? real time command). It
// contains also the temperature override, in case we want to make it more "closed-loop"
class MachineCommunication : public QObject
{
    Q_OBJECT

public:
    MachineCommunication(int hardResetDelay = 1000);

public slots:
    void portFound(MachineInfo info, AbstractPortDiscovery* portDiscoverer);
    void writeData(QByteArray data);
    void writeLine(QByteArray data); // Like writeData but adds \n at the end of data
    void closePortWithError(QString reason);
    void closePort();
    void feedHold();
    void resumeFeedHold();
    void softReset(); // Be careful: after this the firmare will probably go alarm and require a hard reset
    void hardReset(); // Closes and re-open port. Emits machineInitialized

signals:
    void dataSent(QByteArray data);
    void dataReceived(QByteArray data);
    void machineInitialized();
    void portClosedWithError(QString reason);
    void portClosed();

private slots:
    void readData();

private:
    bool checkPortInErrorAndCloseIfTrue();

    const int m_hardResetDelay;
    std::unique_ptr<SerialPortInterface> m_serialPort;
};

#endif // MACHINECOMMUNICATION_H

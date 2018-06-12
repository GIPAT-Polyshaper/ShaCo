#ifndef MACHINECOMMUNICATION_H
#define MACHINECOMMUNICATION_H

#include <memory>
#include <QObject>
#include "portdiscovery.h"
#include "machineinfo.h"
#include "serialport.h"

class MachineCommunication : public QObject
{
    Q_OBJECT

public:
    MachineCommunication();

public slots:
    void portFound(MachineInfo info, AbstractPortDiscovery* portDiscoverer);
    void writeData(QByteArray data);
    void writeLine(QByteArray data); // Like writeData but adds \n at the end of data
    void closePortWithError(QString reason);
    void closePort();

signals:
    void dataSent(QByteArray data);
    void dataReceived(QByteArray data);
    void portOpened();
    void portClosedWithError(QString reason);
    void portClosed();

private slots:
    void readData();

private:
    bool checkPortInErrorAndCloseIfTrue();

    std::unique_ptr<SerialPortInterface> m_serialPort;
};

#endif // MACHINECOMMUNICATION_H

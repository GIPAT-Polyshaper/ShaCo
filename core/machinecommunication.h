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
    void writeLine(QByteArray data);

signals:
    void dataSent(QByteArray data);
    void dataReceived(QByteArray data);

private slots:
    void readData();

private:
    std::unique_ptr<SerialPortInterface> m_serialPort;
};

#endif // MACHINECOMMUNICATION_H

#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <QObject>
#include <QThread>
#include <QSerialPortInfo>
#include "core/machinecommunication.h"
#include "core/machineinfo.h"
#include "core/portdiscovery.h"

class Controller : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)

public:
    explicit Controller(QObject *parent = nullptr);
    virtual ~Controller();

    bool connected() const;

public slots:
    void sendLine(QByteArray line);

signals:
    void startedPortDiscovery();
    void portFound(QString machineName, QString firmwareVersion);
    void connectedChanged();
    void dataSent(QByteArray data);
    void dataReceived(QByteArray data);
    void portClosedWithError(QString reason);
    void portClosed();

private slots:
    void signalPortFound(MachineInfo info);
    void signalPortClosedWithError(QString reason);
    void signalPortClosed();

private:
    void moveToPortThread(QObject* obj);

    QThread m_portThread;
    bool m_connected;
    PortDiscovery<QSerialPortInfo>* const m_portDiscoverer;
    MachineCommunication* const m_machineCommunicator;
};

#endif // CONTROLLER_H

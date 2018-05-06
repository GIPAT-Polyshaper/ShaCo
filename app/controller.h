#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <QObject>
#include <QThread>
#include "core/machineinfo.h"

class Controller : public QObject
{
    Q_OBJECT

public:
    explicit Controller(QObject *parent = nullptr);
    virtual ~Controller();

signals:
    void startedPortDiscovery();
    void portFound(QString machineName, QString firmwareVersion);

private slots:
    void signalPortFound(MachineInfo info);

private:
    QThread m_portThread;
};

#endif // CONTROLLER_H

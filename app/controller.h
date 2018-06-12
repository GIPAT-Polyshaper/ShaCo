#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <QObject>
#include <QThread>
#include <QSerialPortInfo>
#include "core/machinecommunication.h"
#include "core/machineinfo.h"
#include "core/portdiscovery.h"

// TODO-TOMMY Ora agganciare il file sender. Prima bisogna creare delle funzioni qui per ricevere il file da mandare che, oltre a creare e configurare il fileSender. Vedere anche quando distruggerlo! (tramite lo slot deleteLater, visto che fileSender sta in un altro thread). Ricordarsi anche di muovere il QIODevice nel thread worker prima di passarlo al fileSender
// TODO-TOMMY Il coso che setta la temperatura, va agganciato a MachineCommunication prima di fare discovery, altrimenti si perde il segnale di porta aperta e non setta la temperatura iniziale!!!
// TODO-TOMMY Add a test for this class??? If so we should make it template on all components to test connections and signal that are emitted
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

    // TODO-TOMMY Qui slot per settare file. Poi (o prima) aggiungere a MachineCommunication funzioni per incrementare/decrementare la temperatura. Ne servono due tipi: uno da usare prima della lavorazione (Manda comandi gcode normali) e uno da usare durante la lavorazione (con immediate commands, tutti i possibili). Qui mettere solo un tipo di funzioni e chiamare quelle di MachineCommunication a seconda dello stato

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

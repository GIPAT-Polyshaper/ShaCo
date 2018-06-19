#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <QObject>
#include <QThread>
#include <QSerialPortInfo>
#include <QUrl>
#include "core/gcodesender.h"
#include "core/machinecommunication.h"
#include "core/machineinfo.h"
#include "core/portdiscovery.h"
#include "core/wirecontroller.h"

// TODO-TOMMY Ora agganciare il file sender. Prima bisogna creare delle funzioni qui per ricevere il file da mandare che, oltre a creare e configurare il fileSender. Vedere anche quando distruggerlo! (tramite lo slot deleteLater, visto che fileSender sta in un altro thread). Ricordarsi anche di muovere il QIODevice nel thread worker prima di passarlo al fileSender
// TODO-TOMMY Disabilitare il terminale se sto mandando il file, senn? succede un casino
// TODO-TOMMY Add a test for this class??? If so we should make it template on all components to test connections and signal that are emitted
class Controller : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)
    Q_PROPERTY(bool streamingGCode READ streamingGCode NOTIFY streamingGCodeChanged)
    Q_PROPERTY(bool wireOn READ wireOn WRITE setWireOn NOTIFY wireOnChanged)
    Q_PROPERTY(float wireTemperature READ wireTemperature WRITE setWireTemperature NOTIFY wireTemperatureChanged)

public:
    explicit Controller(QObject *parent = nullptr);
    virtual ~Controller();

    bool connected() const;
    bool streamingGCode() const;
    bool wireOn() const;
    float wireTemperature() const;

public slots:
    void sendLine(QByteArray line);
    void setGCodeFile(QUrl fileUrl);
    void setWireOn(bool wireOn);
    void setWireTemperature(float temperature);
    void startStreamingGCode();

    //AGGIUNGERE CODICE PER FAR FUNZIONARE STOP, PAUSE E PER FERMARE TUTTO QUANDO SI CHIUDE IL PROGRAMMA (METTERE DIALOG CON "OK USCIRE"? SE SI STA TAGLIANDO E SI CERCA DI CHIUDERE LA FINESTRA). POI SISTEMARE: SE STO TAGLIANDO NON DEVO PASSARE PER PRE-CUT PER TORNARE ALLA PAGINA DEL CUT E PULSANTE IMPORT DEVE ESSERE DISABILITATO. NELLA FINESTRA DI CUT AGGIUNGERE TESTO: "PREVIEW NOT AVAILABLE" E POI LA PROGRESS BAR FARLA PROGRESS INDETERMINATA.

signals:
    void startedPortDiscovery();
    void portFound(QString machineName, QString firmwareVersion);
    void connectedChanged();
    void dataSent(QByteArray data);
    void dataReceived(QByteArray data);
    void portClosedWithError(QString reason);
    void portClosed();
    void streamingGCodeChanged();
    void wireOnChanged();
    void wireTemperatureChanged();

private slots:
    void signalPortFound(MachineInfo info);
    void signalPortClosedWithError(QString reason);
    void signalPortClosed();
    void streamingStarted();
    void streamingEnded(GCodeSender::StreamEndReason reason, QString description);

private:
    void moveToPortThread(QObject* obj);

    QThread m_portThread;
    PortDiscovery<QSerialPortInfo>* const m_portDiscoverer;
    MachineCommunication* const m_machineCommunicator;
    WireController* const m_wireController;
    GCodeSender* m_gcodeSender;
    bool m_connected;
    bool m_streamingGCode;
};

#endif // CONTROLLER_H

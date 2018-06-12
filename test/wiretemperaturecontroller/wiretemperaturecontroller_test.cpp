#include <QCoreApplication>
#include <QSignalSpy>
#include <QtTest>
#include "core/machinecommunication.h"
#include "core/wiretemperaturecontroller.h"
#include "testcommon/testportdiscovery.h"
#include "testcommon/testserialport.h"
#include "testcommon/utils.h"

class WireTemperatureControllerTest : public QObject
{
    Q_OBJECT

public:
    WireTemperatureControllerTest();

private Q_SLOTS:
    void setInitialTemperatureAndEmitSignalWhenPortIsOpened();
    void setTemperatureWithGCodeCommand();
    void switchWireOn();
    void doNotEmitSignalWhenSwitchWireOnIsCalledAndWireIsAlreadyOn();
    void switchWireOff();
    void doNotEmitSignalWhenSwitchWireOffIsCalledAndWireIsAlreadyOff();
    void computeTheMinumAndMaximumValuesForSetRealTimeTemperature();
    void setRealTimeTemperatureToALowerTemperature();
    void setRealTimeTemperatureToAHigherTemperature();
    void doNotAllowReatTimeTemperaturesLowerThanMinRealTimeTemperature();
    void doNotAllowReatTimeTemperaturesHIgherThanMaxRealTimeTemperature();
    void setRealTimeTemperatureMoreThanOnce();
    void whenSettingRealTimeTemperatureAfterACallToSetTemperatureStartFromResetRealTimeOverride();
    void setRealTimeTemperatureDoesNotEmitSignalIfTemperatureDoesNotChange();
    void resetRealTimeTemperature();
    void resetRealTimeTemperatureDoesNothingIfRealTimeTemperatureIsEqualToBaseTemperature();
    void setRealTimeTemperatureAfterResetRealTimeTemperatureWorksCorrectly();
    void returnTheCurrentTemperature();
};

WireTemperatureControllerTest::WireTemperatureControllerTest()
{
}

void WireTemperatureControllerTest::setInitialTemperatureAndEmitSignalWhenPortIsOpened()
{
    auto serialPort = new TestSerialPort();
    TestPortDiscovery portDiscoverer(serialPort);
    auto communicator = std::make_unique<MachineCommunication>();

    QSignalSpy dataSentSpy(communicator.get(), &MachineCommunication::dataSent);

    WireTemperatureController temperatureController(communicator.get());

    QSignalSpy temperatureChangedSpy(&temperatureController, &WireTemperatureController::temperatureChanged);

    communicator->portFound(MachineInfo("a", "1"), &portDiscoverer);

    QCOMPARE(dataSentSpy.count(), 1);
    auto data = dataSentSpy.at(0).at(0).toByteArray();
    QCOMPARE(data, "\x99S30\n");
    QCOMPARE(temperatureChangedSpy.count(), 1);
    auto temperature = temperatureChangedSpy.at(0).at(0).toFloat();
    QCOMPARE(temperature, 30.0f);
}

void WireTemperatureControllerTest::setTemperatureWithGCodeCommand()
{
    auto communicator = std::move(createCommunicator().first);

    QSignalSpy dataSentSpy(communicator.get(), &MachineCommunication::dataSent);

    WireTemperatureController temperatureController(communicator.get());

    QSignalSpy temperatureChangedSpy(&temperatureController, &WireTemperatureController::temperatureChanged);

    temperatureController.setTemperature(45.6f);

    QCOMPARE(dataSentSpy.count(), 1);
    auto data = dataSentSpy.at(0).at(0).toByteArray();
    QCOMPARE(data, "\x99S46\n");
    QCOMPARE(temperatureChangedSpy.count(), 1);
    auto temperature = temperatureChangedSpy.at(0).at(0).toFloat();
    QCOMPARE(temperature, 45.6f);
}

void WireTemperatureControllerTest::switchWireOn()
{
    auto communicator = std::move(createCommunicator().first);

    QSignalSpy dataSentSpy(communicator.get(), &MachineCommunication::dataSent);

    WireTemperatureController temperatureController(communicator.get());

    QSignalSpy wireOnSpy(&temperatureController, &WireTemperatureController::wireOn);

    QVERIFY(!temperatureController.isWireOn());
    temperatureController.switchWireOn();

    QCOMPARE(dataSentSpy.count(), 1);
    auto data = dataSentSpy.at(0).at(0).toByteArray();
    QCOMPARE(data, "M3\n");
    QCOMPARE(wireOnSpy.count(), 1);
    QVERIFY(temperatureController.isWireOn());
}

void WireTemperatureControllerTest::doNotEmitSignalWhenSwitchWireOnIsCalledAndWireIsAlreadyOn()
{
    auto communicator = std::move(createCommunicator().first);

    WireTemperatureController temperatureController(communicator.get());

    QSignalSpy wireOnSpy(&temperatureController, &WireTemperatureController::wireOn);

    // Called twice, just one signal emitted
    temperatureController.switchWireOn();
    temperatureController.switchWireOn();

    QCOMPARE(wireOnSpy.count(), 1);
}

void WireTemperatureControllerTest::switchWireOff()
{
    auto communicator = std::move(createCommunicator().first);

    QSignalSpy dataSentSpy(communicator.get(), &MachineCommunication::dataSent);

    WireTemperatureController temperatureController(communicator.get());

    QSignalSpy wireOffSpy(&temperatureController, &WireTemperatureController::wireOff);

    temperatureController.switchWireOn();
    temperatureController.switchWireOff();

    QCOMPARE(dataSentSpy.count(), 2); // one for on, the other for off
    auto data = dataSentSpy.at(1).at(0).toByteArray();
    QCOMPARE(data, "M5\n");
    QCOMPARE(wireOffSpy.count(), 1);
    QVERIFY(!temperatureController.isWireOn());
}

void WireTemperatureControllerTest::doNotEmitSignalWhenSwitchWireOffIsCalledAndWireIsAlreadyOff()
{
    auto communicator = std::move(createCommunicator().first);

    WireTemperatureController temperatureController(communicator.get());

    QSignalSpy wireOffSpy(&temperatureController, &WireTemperatureController::wireOff);

    // Wire is off, no signal
    temperatureController.switchWireOff();

    QCOMPARE(wireOffSpy.count(), 0);
}

void WireTemperatureControllerTest::computeTheMinumAndMaximumValuesForSetRealTimeTemperature()
{
    auto communicator = std::move(createCommunicator().first);

    WireTemperatureController temperatureController(communicator.get());

    temperatureController.setTemperature(40.0f);

    QCOMPARE(temperatureController.minRealTimeTemperature(), 4.0f);
    QCOMPARE(temperatureController.maxRealTimeTemperature(), 80.0f);
}

void WireTemperatureControllerTest::setRealTimeTemperatureToALowerTemperature()
{
    auto communicator = std::move(createCommunicator().first);

    WireTemperatureController temperatureController(communicator.get());
    temperatureController.setTemperature(40.0f);

    QSignalSpy dataSentSpy(communicator.get(), &MachineCommunication::dataSent);
    QSignalSpy temperatureChangedSpy(&temperatureController, &WireTemperatureController::temperatureChanged);

    temperatureController.setRealTimeTemperature(30.0f);

    QCOMPARE(dataSentSpy.count(), 1);
    QCOMPARE(dataSentSpy.at(0).at(0).toByteArray(), "\x9B\x9B\x9D\x9D\x9D\x9D\x9D");

    QCOMPARE(temperatureChangedSpy.count(), 1);
    auto temperature = temperatureChangedSpy.at(0).at(0).toFloat();
    QCOMPARE(temperature, 30.0f);
}

void WireTemperatureControllerTest::setRealTimeTemperatureToAHigherTemperature()
{
    auto communicator = std::move(createCommunicator().first);

    WireTemperatureController temperatureController(communicator.get());
    temperatureController.setTemperature(40.0f);

    QSignalSpy dataSentSpy(communicator.get(), &MachineCommunication::dataSent);
    QSignalSpy temperatureChangedSpy(&temperatureController, &WireTemperatureController::temperatureChanged);

    temperatureController.setRealTimeTemperature(46.8f);

    QCOMPARE(dataSentSpy.count(), 1);
    QCOMPARE(dataSentSpy.at(0).at(0).toByteArray(), "\x9A\x9C\x9C\x9C\x9C\x9C\x9C\x9C");

    QCOMPARE(temperatureChangedSpy.count(), 1);
    auto temperature = temperatureChangedSpy.at(0).at(0).toFloat();
    QCOMPARE(temperature, 46.8f);
}

void WireTemperatureControllerTest::doNotAllowReatTimeTemperaturesLowerThanMinRealTimeTemperature()
{
    auto communicator = std::move(createCommunicator().first);

    WireTemperatureController temperatureController(communicator.get());
    temperatureController.setTemperature(40.0f);

    QSignalSpy dataSentSpy(communicator.get(), &MachineCommunication::dataSent);
    QSignalSpy temperatureChangedSpy(&temperatureController, &WireTemperatureController::temperatureChanged);

    temperatureController.setRealTimeTemperature(1.0f);

    QCOMPARE(dataSentSpy.count(), 1);
    QCOMPARE(dataSentSpy.at(0).at(0).toByteArray(), "\x9B\x9B\x9B\x9B\x9B\x9B\x9B\x9B\x9B");

    QCOMPARE(temperatureChangedSpy.count(), 1);
    auto temperature = temperatureChangedSpy.at(0).at(0).toFloat();
    QCOMPARE(temperature, 4.0f);
}

void WireTemperatureControllerTest::doNotAllowReatTimeTemperaturesHIgherThanMaxRealTimeTemperature()
{
    auto communicator = std::move(createCommunicator().first);

    WireTemperatureController temperatureController(communicator.get());
    temperatureController.setTemperature(40.0f);

    QSignalSpy dataSentSpy(communicator.get(), &MachineCommunication::dataSent);
    QSignalSpy temperatureChangedSpy(&temperatureController, &WireTemperatureController::temperatureChanged);

    temperatureController.setRealTimeTemperature(100.0f);

    QCOMPARE(dataSentSpy.count(), 1);
    QCOMPARE(dataSentSpy.at(0).at(0).toByteArray(), "\x9A\x9A\x9A\x9A\x9A\x9A\x9A\x9A\x9A\x9A");

    QCOMPARE(temperatureChangedSpy.count(), 1);
    auto temperature = temperatureChangedSpy.at(0).at(0).toFloat();
    QCOMPARE(temperature, 80.0f);
}

void WireTemperatureControllerTest::setRealTimeTemperatureMoreThanOnce()
{
    auto communicator = std::move(createCommunicator().first);

    WireTemperatureController temperatureController(communicator.get());
    temperatureController.setTemperature(40.0f);

    temperatureController.setRealTimeTemperature(30.0f);

    QSignalSpy dataSentSpy(communicator.get(), &MachineCommunication::dataSent);
    QSignalSpy temperatureChangedSpy(&temperatureController, &WireTemperatureController::temperatureChanged);

    temperatureController.setRealTimeTemperature(10.0f);

    QCOMPARE(dataSentSpy.count(), 1);
    QCOMPARE(dataSentSpy.at(0).at(0).toByteArray(), "\x9B\x9B\x9B\x9B\x9B");

    QCOMPARE(temperatureChangedSpy.count(), 1);
    auto temperature = temperatureChangedSpy.at(0).at(0).toFloat();
    QCOMPARE(temperature, 10.0f);
}

void WireTemperatureControllerTest::whenSettingRealTimeTemperatureAfterACallToSetTemperatureStartFromResetRealTimeOverride()
{
    auto communicator = std::move(createCommunicator().first);

    WireTemperatureController temperatureController(communicator.get());
    temperatureController.setTemperature(40.0f);

    temperatureController.setRealTimeTemperature(30.0f);
    temperatureController.setTemperature(50.0f);

    QSignalSpy dataSentSpy(communicator.get(), &MachineCommunication::dataSent);
    QSignalSpy temperatureChangedSpy(&temperatureController, &WireTemperatureController::temperatureChanged);

    temperatureController.setRealTimeTemperature(20.0f);

    QCOMPARE(dataSentSpy.count(), 1);
    QCOMPARE(dataSentSpy.at(0).at(0).toByteArray(), "\x9B\x9B\x9B\x9B\x9B\x9B");

    QCOMPARE(temperatureChangedSpy.count(), 1);
    auto temperature = temperatureChangedSpy.at(0).at(0).toFloat();
    QCOMPARE(temperature, 20.0f);
}

void WireTemperatureControllerTest::setRealTimeTemperatureDoesNotEmitSignalIfTemperatureDoesNotChange()
{
    auto communicator = std::move(createCommunicator().first);

    WireTemperatureController temperatureController(communicator.get());
    temperatureController.setTemperature(40.0f);

    temperatureController.setRealTimeTemperature(30.0f);

    QSignalSpy dataSentSpy(communicator.get(), &MachineCommunication::dataSent);
    QSignalSpy temperatureChangedSpy(&temperatureController, &WireTemperatureController::temperatureChanged);

    temperatureController.setRealTimeTemperature(30.0f);

    QCOMPARE(dataSentSpy.count(), 0);
    QCOMPARE(temperatureChangedSpy.count(), 0);
}

void WireTemperatureControllerTest::resetRealTimeTemperature()
{
    auto communicator = std::move(createCommunicator().first);

    WireTemperatureController temperatureController(communicator.get());
    temperatureController.setTemperature(40.0f);

    temperatureController.setRealTimeTemperature(30.0f);

    QSignalSpy dataSentSpy(communicator.get(), &MachineCommunication::dataSent);
    QSignalSpy temperatureChangedSpy(&temperatureController, &WireTemperatureController::temperatureChanged);

    temperatureController.resetRealTimeTemperature();

    QCOMPARE(dataSentSpy.count(), 1);
    QCOMPARE(dataSentSpy.at(0).at(0).toByteArray(), "\x99");

    QCOMPARE(temperatureChangedSpy.count(), 1);
    auto temperature = temperatureChangedSpy.at(0).at(0).toFloat();
    QCOMPARE(temperature, 40.0f);
}

void WireTemperatureControllerTest::resetRealTimeTemperatureDoesNothingIfRealTimeTemperatureIsEqualToBaseTemperature()
{
    auto communicator = std::move(createCommunicator().first);

    WireTemperatureController temperatureController(communicator.get());
    temperatureController.setTemperature(40.0f);

    QSignalSpy dataSentSpy(communicator.get(), &MachineCommunication::dataSent);
    QSignalSpy temperatureChangedSpy(&temperatureController, &WireTemperatureController::temperatureChanged);

    temperatureController.resetRealTimeTemperature();

    QCOMPARE(dataSentSpy.count(), 0);
    QCOMPARE(temperatureChangedSpy.count(), 0);
}

void WireTemperatureControllerTest::setRealTimeTemperatureAfterResetRealTimeTemperatureWorksCorrectly()
{
    auto communicator = std::move(createCommunicator().first);

    WireTemperatureController temperatureController(communicator.get());
    temperatureController.setTemperature(40.0f);

    temperatureController.setRealTimeTemperature(30.0f);
    temperatureController.resetRealTimeTemperature();

    QSignalSpy dataSentSpy(communicator.get(), &MachineCommunication::dataSent);
    QSignalSpy temperatureChangedSpy(&temperatureController, &WireTemperatureController::temperatureChanged);

    temperatureController.setRealTimeTemperature(20.0f);

    QCOMPARE(dataSentSpy.count(), 1);
    QCOMPARE(dataSentSpy.at(0).at(0).toByteArray(), "\x9B\x9B\x9B\x9B\x9B");

    QCOMPARE(temperatureChangedSpy.count(), 1);
    auto temperature = temperatureChangedSpy.at(0).at(0).toFloat();
    QCOMPARE(temperature, 20.0f);
}

void WireTemperatureControllerTest::returnTheCurrentTemperature()
{
    auto communicator = std::move(createCommunicator().first);

    WireTemperatureController temperatureController(communicator.get());
    temperatureController.setTemperature(40.0f);

    temperatureController.setRealTimeTemperature(30.0f);

    QCOMPARE(temperatureController.temperature(), 30.0f);
}

QTEST_GUILESS_MAIN(WireTemperatureControllerTest)

#include "wiretemperaturecontroller_test.moc"

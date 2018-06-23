#include <QCoreApplication>
#include <QSignalSpy>
#include <QtTest>
#include "core/machinecommunication.h"
#include "core/wirecontroller.h"
#include "testcommon/testportdiscovery.h"
#include "testcommon/testserialport.h"
#include "testcommon/utils.h"

class WireControllerTest : public QObject
{
    Q_OBJECT

public:
    WireControllerTest();

private Q_SLOTS:
    void setInitialTemperatureAndEmitSignalWhenMachineIsInitialized();
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
    void returnTheLengthOfTheSwitchWireOnCommand();
    void returnTheLengthOfTheSwitchWireOffCommand();
    void whenMachineInitializedSignalIsReceivedSwitchWireOffAndSetBaseTemperatureToCurrentTemperature();
};

WireControllerTest::WireControllerTest()
{
}

void WireControllerTest::setInitialTemperatureAndEmitSignalWhenMachineIsInitialized()
{
    auto serialPort = new TestSerialPort();
    TestPortDiscovery portDiscoverer(serialPort);
    auto communicator = std::make_unique<MachineCommunication>();

    QSignalSpy dataSentSpy(communicator.get(), &MachineCommunication::dataSent);

    WireController wireController(communicator.get());

    QSignalSpy temperatureChangedSpy(&wireController, &WireController::temperatureChanged);
    QSignalSpy wireOffSpy(&wireController, &WireController::wireOff);

    communicator->portFound(MachineInfo("a", "1"), &portDiscoverer);

    QCOMPARE(dataSentSpy.count(), 2);
    QCOMPARE(dataSentSpy.at(0).at(0).toByteArray(), "M5\n");
    QCOMPARE(dataSentSpy.at(1).at(0).toByteArray(), "\x99S30\n");
    QCOMPARE(wireOffSpy.count(), 1);
    QCOMPARE(temperatureChangedSpy.count(), 1);
    auto temperature = temperatureChangedSpy.at(0).at(0).toFloat();
    QCOMPARE(temperature, 30.0f);
}

void WireControllerTest::setTemperatureWithGCodeCommand()
{
    auto communicator = std::move(createCommunicator().first);

    QSignalSpy dataSentSpy(communicator.get(), &MachineCommunication::dataSent);

    WireController wireController(communicator.get());

    QSignalSpy temperatureChangedSpy(&wireController, &WireController::temperatureChanged);

    wireController.setTemperature(45.6f);

    QCOMPARE(dataSentSpy.count(), 1);
    auto data = dataSentSpy.at(0).at(0).toByteArray();
    QCOMPARE(data, "\x99S46\n");
    QCOMPARE(temperatureChangedSpy.count(), 1);
    auto temperature = temperatureChangedSpy.at(0).at(0).toFloat();
    QCOMPARE(temperature, 45.6f);
}

void WireControllerTest::switchWireOn()
{
    auto communicator = std::move(createCommunicator().first);

    QSignalSpy dataSentSpy(communicator.get(), &MachineCommunication::dataSent);

    WireController wireController(communicator.get());

    QSignalSpy wireOnSpy(&wireController, &WireController::wireOn);

    QVERIFY(!wireController.isWireOn());
    wireController.switchWireOn();

    QCOMPARE(dataSentSpy.count(), 1);
    auto data = dataSentSpy.at(0).at(0).toByteArray();
    QCOMPARE(data, "M3\n");
    QCOMPARE(wireOnSpy.count(), 1);
    QVERIFY(wireController.isWireOn());
}

void WireControllerTest::doNotEmitSignalWhenSwitchWireOnIsCalledAndWireIsAlreadyOn()
{
    auto communicator = std::move(createCommunicator().first);

    WireController wireController(communicator.get());

    QSignalSpy wireOnSpy(&wireController, &WireController::wireOn);

    // Called twice, just one signal emitted
    wireController.switchWireOn();
    wireController.switchWireOn();

    QCOMPARE(wireOnSpy.count(), 1);
}

void WireControllerTest::switchWireOff()
{
    auto communicator = std::move(createCommunicator().first);

    QSignalSpy dataSentSpy(communicator.get(), &MachineCommunication::dataSent);

    WireController wireController(communicator.get());

    QSignalSpy wireOffSpy(&wireController, &WireController::wireOff);

    wireController.switchWireOn();
    wireController.switchWireOff();

    QCOMPARE(dataSentSpy.count(), 2); // one for on, the other for off
    auto data = dataSentSpy.at(1).at(0).toByteArray();
    QCOMPARE(data, "M5\n");
    QCOMPARE(wireOffSpy.count(), 1);
    QVERIFY(!wireController.isWireOn());
}

void WireControllerTest::doNotEmitSignalWhenSwitchWireOffIsCalledAndWireIsAlreadyOff()
{
    auto communicator = std::move(createCommunicator().first);

    WireController wireController(communicator.get());

    QSignalSpy wireOffSpy(&wireController, &WireController::wireOff);

    // Wire is off, no signal
    wireController.switchWireOff();

    QCOMPARE(wireOffSpy.count(), 0);
}

void WireControllerTest::computeTheMinumAndMaximumValuesForSetRealTimeTemperature()
{
    auto communicator = std::move(createCommunicator().first);

    WireController wireController(communicator.get());

    wireController.setTemperature(40.0f);

    QCOMPARE(wireController.minRealTimeTemperature(), 4.0f);
    QCOMPARE(wireController.maxRealTimeTemperature(), 80.0f);
}

void WireControllerTest::setRealTimeTemperatureToALowerTemperature()
{
    auto communicator = std::move(createCommunicator().first);

    WireController wireController(communicator.get());
    wireController.setTemperature(40.0f);

    QSignalSpy dataSentSpy(communicator.get(), &MachineCommunication::dataSent);
    QSignalSpy temperatureChangedSpy(&wireController, &WireController::temperatureChanged);

    wireController.setRealTimeTemperature(30.0f);

    QCOMPARE(dataSentSpy.count(), 1);
    QCOMPARE(dataSentSpy.at(0).at(0).toByteArray(), "\x9B\x9B\x9D\x9D\x9D\x9D\x9D");

    QCOMPARE(temperatureChangedSpy.count(), 1);
    auto temperature = temperatureChangedSpy.at(0).at(0).toFloat();
    QCOMPARE(temperature, 30.0f);
}

void WireControllerTest::setRealTimeTemperatureToAHigherTemperature()
{
    auto communicator = std::move(createCommunicator().first);

    WireController wireController(communicator.get());
    wireController.setTemperature(40.0f);

    QSignalSpy dataSentSpy(communicator.get(), &MachineCommunication::dataSent);
    QSignalSpy temperatureChangedSpy(&wireController, &WireController::temperatureChanged);

    wireController.setRealTimeTemperature(46.8f);

    QCOMPARE(dataSentSpy.count(), 1);
    QCOMPARE(dataSentSpy.at(0).at(0).toByteArray(), "\x9A\x9C\x9C\x9C\x9C\x9C\x9C\x9C");

    QCOMPARE(temperatureChangedSpy.count(), 1);
    auto temperature = temperatureChangedSpy.at(0).at(0).toFloat();
    QCOMPARE(temperature, 46.8f);
}

void WireControllerTest::doNotAllowReatTimeTemperaturesLowerThanMinRealTimeTemperature()
{
    auto communicator = std::move(createCommunicator().first);

    WireController wireController(communicator.get());
    wireController.setTemperature(40.0f);

    QSignalSpy dataSentSpy(communicator.get(), &MachineCommunication::dataSent);
    QSignalSpy temperatureChangedSpy(&wireController, &WireController::temperatureChanged);

    wireController.setRealTimeTemperature(1.0f);

    QCOMPARE(dataSentSpy.count(), 1);
    QCOMPARE(dataSentSpy.at(0).at(0).toByteArray(), "\x9B\x9B\x9B\x9B\x9B\x9B\x9B\x9B\x9B");

    QCOMPARE(temperatureChangedSpy.count(), 1);
    auto temperature = temperatureChangedSpy.at(0).at(0).toFloat();
    QCOMPARE(temperature, 4.0f);
}

void WireControllerTest::doNotAllowReatTimeTemperaturesHIgherThanMaxRealTimeTemperature()
{
    auto communicator = std::move(createCommunicator().first);

    WireController wireController(communicator.get());
    wireController.setTemperature(40.0f);

    QSignalSpy dataSentSpy(communicator.get(), &MachineCommunication::dataSent);
    QSignalSpy temperatureChangedSpy(&wireController, &WireController::temperatureChanged);

    wireController.setRealTimeTemperature(100.0f);

    QCOMPARE(dataSentSpy.count(), 1);
    QCOMPARE(dataSentSpy.at(0).at(0).toByteArray(), "\x9A\x9A\x9A\x9A\x9A\x9A\x9A\x9A\x9A\x9A");

    QCOMPARE(temperatureChangedSpy.count(), 1);
    auto temperature = temperatureChangedSpy.at(0).at(0).toFloat();
    QCOMPARE(temperature, 80.0f);
}

void WireControllerTest::setRealTimeTemperatureMoreThanOnce()
{
    auto communicator = std::move(createCommunicator().first);

    WireController wireController(communicator.get());
    wireController.setTemperature(40.0f);

    wireController.setRealTimeTemperature(30.0f);

    QSignalSpy dataSentSpy(communicator.get(), &MachineCommunication::dataSent);
    QSignalSpy temperatureChangedSpy(&wireController, &WireController::temperatureChanged);

    wireController.setRealTimeTemperature(10.0f);

    QCOMPARE(dataSentSpy.count(), 1);
    QCOMPARE(dataSentSpy.at(0).at(0).toByteArray(), "\x9B\x9B\x9B\x9B\x9B");

    QCOMPARE(temperatureChangedSpy.count(), 1);
    auto temperature = temperatureChangedSpy.at(0).at(0).toFloat();
    QCOMPARE(temperature, 10.0f);
}

void WireControllerTest::whenSettingRealTimeTemperatureAfterACallToSetTemperatureStartFromResetRealTimeOverride()
{
    auto communicator = std::move(createCommunicator().first);

    WireController wireController(communicator.get());
    wireController.setTemperature(40.0f);

    wireController.setRealTimeTemperature(30.0f);
    wireController.setTemperature(50.0f);

    QSignalSpy dataSentSpy(communicator.get(), &MachineCommunication::dataSent);
    QSignalSpy temperatureChangedSpy(&wireController, &WireController::temperatureChanged);

    wireController.setRealTimeTemperature(20.0f);

    QCOMPARE(dataSentSpy.count(), 1);
    QCOMPARE(dataSentSpy.at(0).at(0).toByteArray(), "\x9B\x9B\x9B\x9B\x9B\x9B");

    QCOMPARE(temperatureChangedSpy.count(), 1);
    auto temperature = temperatureChangedSpy.at(0).at(0).toFloat();
    QCOMPARE(temperature, 20.0f);
}

void WireControllerTest::setRealTimeTemperatureDoesNotEmitSignalIfTemperatureDoesNotChange()
{
    auto communicator = std::move(createCommunicator().first);

    WireController wireController(communicator.get());
    wireController.setTemperature(40.0f);

    wireController.setRealTimeTemperature(30.0f);

    QSignalSpy dataSentSpy(communicator.get(), &MachineCommunication::dataSent);
    QSignalSpy temperatureChangedSpy(&wireController, &WireController::temperatureChanged);

    wireController.setRealTimeTemperature(30.0f);

    QCOMPARE(dataSentSpy.count(), 0);
    QCOMPARE(temperatureChangedSpy.count(), 0);
}

void WireControllerTest::resetRealTimeTemperature()
{
    auto communicator = std::move(createCommunicator().first);

    WireController wireController(communicator.get());
    wireController.setTemperature(40.0f);

    wireController.setRealTimeTemperature(30.0f);

    QSignalSpy dataSentSpy(communicator.get(), &MachineCommunication::dataSent);
    QSignalSpy temperatureChangedSpy(&wireController, &WireController::temperatureChanged);

    wireController.resetRealTimeTemperature();

    QCOMPARE(dataSentSpy.count(), 1);
    QCOMPARE(dataSentSpy.at(0).at(0).toByteArray(), "\x99");

    QCOMPARE(temperatureChangedSpy.count(), 1);
    auto temperature = temperatureChangedSpy.at(0).at(0).toFloat();
    QCOMPARE(temperature, 40.0f);
}

void WireControllerTest::resetRealTimeTemperatureDoesNothingIfRealTimeTemperatureIsEqualToBaseTemperature()
{
    auto communicator = std::move(createCommunicator().first);

    WireController wireController(communicator.get());
    wireController.setTemperature(40.0f);

    QSignalSpy dataSentSpy(communicator.get(), &MachineCommunication::dataSent);
    QSignalSpy temperatureChangedSpy(&wireController, &WireController::temperatureChanged);

    wireController.resetRealTimeTemperature();

    QCOMPARE(dataSentSpy.count(), 0);
    QCOMPARE(temperatureChangedSpy.count(), 0);
}

void WireControllerTest::setRealTimeTemperatureAfterResetRealTimeTemperatureWorksCorrectly()
{
    auto communicator = std::move(createCommunicator().first);

    WireController wireController(communicator.get());
    wireController.setTemperature(40.0f);

    wireController.setRealTimeTemperature(30.0f);
    wireController.resetRealTimeTemperature();

    QSignalSpy dataSentSpy(communicator.get(), &MachineCommunication::dataSent);
    QSignalSpy temperatureChangedSpy(&wireController, &WireController::temperatureChanged);

    wireController.setRealTimeTemperature(20.0f);

    QCOMPARE(dataSentSpy.count(), 1);
    QCOMPARE(dataSentSpy.at(0).at(0).toByteArray(), "\x9B\x9B\x9B\x9B\x9B");

    QCOMPARE(temperatureChangedSpy.count(), 1);
    auto temperature = temperatureChangedSpy.at(0).at(0).toFloat();
    QCOMPARE(temperature, 20.0f);
}

void WireControllerTest::returnTheCurrentTemperature()
{
    auto communicator = std::move(createCommunicator().first);

    WireController wireController(communicator.get());
    wireController.setTemperature(40.0f);

    wireController.setRealTimeTemperature(30.0f);

    QCOMPARE(wireController.temperature(), 30.0f);
}

void WireControllerTest::returnTheLengthOfTheSwitchWireOnCommand()
{
    auto communicator = std::move(createCommunicator().first);

    WireController wireController(communicator.get());

    QCOMPARE(wireController.switchWireOnCommandLength(), 3);
}

void WireControllerTest::returnTheLengthOfTheSwitchWireOffCommand()
{
    auto communicator = std::move(createCommunicator().first);

    WireController wireController(communicator.get());

    QCOMPARE(wireController.switchWireOffCommandLength(), 3);
}

void WireControllerTest::whenMachineInitializedSignalIsReceivedSwitchWireOffAndSetBaseTemperatureToCurrentTemperature()
{
    auto serialPort = new TestSerialPort();
    TestPortDiscovery portDiscoverer(serialPort);
    auto communicator = std::make_unique<MachineCommunication>(100);

    QSignalSpy dataSentSpy(communicator.get(), &MachineCommunication::dataSent);

    WireController wireController(communicator.get());

    QSignalSpy temperatureChangedSpy(&wireController, &WireController::temperatureChanged);
    QSignalSpy wireOffSpy(&wireController, &WireController::wireOff);

    communicator->portFound(MachineInfo("a", "1"), &portDiscoverer);

    QCOMPARE(dataSentSpy.count(), 2);
    QCOMPARE(dataSentSpy.at(0).at(0).toByteArray(), "M5\n");
    QCOMPARE(dataSentSpy.at(1).at(0).toByteArray(), "\x99S30\n");
    QCOMPARE(wireOffSpy.count(), 1);
    QCOMPARE(temperatureChangedSpy.count(), 1);
    QCOMPARE(temperatureChangedSpy.at(0).at(0).toFloat(), 30.0f);

    wireController.setTemperature(11.1f);
    QCOMPARE(temperatureChangedSpy.at(1).at(0).toFloat(), 11.1f);
    wireController.setRealTimeTemperature(14.3f);
    QCOMPARE(temperatureChangedSpy.at(2).at(0).toFloat(), 14.319f); // Not exactly 14.3f because of approximations
    communicator->hardReset(); // This causes the machineInitialized signal to be sent

    QCOMPARE(dataSentSpy.count(), 6);
    QCOMPARE(dataSentSpy.at(4).at(0).toByteArray(), "M5\n");
    QCOMPARE(dataSentSpy.at(5).at(0).toByteArray(), "\x99S14\n");
    QCOMPARE(wireOffSpy.count(), 2);
    QCOMPARE(temperatureChangedSpy.count(), 4);
    QCOMPARE(temperatureChangedSpy.at(3).at(0).toFloat(), 14.319f);
}

QTEST_GUILESS_MAIN(WireControllerTest)

#include "wirecontroller_test.moc"

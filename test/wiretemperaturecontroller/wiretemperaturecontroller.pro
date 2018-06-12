# Check the config files exist
!include(../test.pri) {
    error("Couldn't find the test.pri file!")
}

TARGET = wiretemperaturecontroller_test

SOURCES += \
        wiretemperaturecontroller_test.cpp

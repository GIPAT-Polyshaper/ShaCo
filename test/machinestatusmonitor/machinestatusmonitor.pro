# Check the config files exist
!include(../test.pri) {
    error("Couldn't find the test.pri file!")
}

TARGET = machinestatusmonitor_test

SOURCES += \
        machinestatusmonitor_test.cpp

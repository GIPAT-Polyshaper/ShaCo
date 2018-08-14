# Check the config files exist
!include(../test.pri) {
    error("Couldn't find the test.pri file!")
}

TARGET = machinestate_test

SOURCES += machinestate_test.cpp

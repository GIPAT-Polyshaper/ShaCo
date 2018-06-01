# Check the config files exist
!include(../test.pri) {
    error("Couldn't find the test.pri file!")
}

TARGET = machineinfo_test

SOURCES += machineinfo_test.cpp

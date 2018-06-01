# Check the config files exist
!include(../test.pri) {
    error("Couldn't find the test.pri file!")
}

TARGET = machinecommunication_test

SOURCES += machinecommunication_test.cpp

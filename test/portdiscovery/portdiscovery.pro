# Check the config files exist
!include(../test.pri) {
    error("Couldn't find the test.pri file!")
}

TARGET = portdiscovery_test

SOURCES += portdiscovery_test.cpp

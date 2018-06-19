# Check the config files exist
!include(../test.pri) {
    error("Couldn't find the test.pri file!")
}

TARGET = wirecontroller_test

SOURCES += \
    wirecontroller_test.cpp

# Check the config files exist
!include(../test.pri) {
    error("Couldn't find the test.pri file!")
}

TARGET = gcodesender_test

SOURCES += \
        gcodesender_test.cpp

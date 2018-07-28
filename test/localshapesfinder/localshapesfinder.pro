# Check the config files exist
!include(../test.pri) {
    error("Couldn't find the test.pri file!")
}

TARGET = localshapesfinder_test

SOURCES += \
        localshapesfinder_test.cpp

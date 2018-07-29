# Check the config files exist
!include(../test.pri) {
    error("Couldn't find the test.pri file!")
}

TARGET = shapeinfo_test

SOURCES += \
        shapeinfo_test.cpp

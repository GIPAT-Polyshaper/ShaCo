# Check the config files exist
!include(../../common.pri) {
    error("Couldn't find the common.pri file!")
}

TARGET = testcommon
TEMPLATE = lib
CONFIG += staticlib
QT += serialport
QT -= gui

INCLUDEPATH += ../..
LIBS += -L../../core -lcore

HEADERS += \
    testportdiscovery.h \
    testserialport.h
SOURCES += \
    testportdiscovery.cpp \
    testserialport.cpp

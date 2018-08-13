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
unix:LIBS += -L../core -lcore
win32:debug:LIBS += -L../core/debug -lcore
win32:release:LIBS += -L../core/release -lcore

HEADERS += \
    testportdiscovery.h \
    testserialport.h \
    utils.h \
    testmachineinfo.h
SOURCES += \
    testportdiscovery.cpp \
    testserialport.cpp \
    utils.cpp \
    testmachineinfo.cpp

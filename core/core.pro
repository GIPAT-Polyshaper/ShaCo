# Check if the config file exists
!include(../common.pri) {
    error("Couldn't find the common.pri file!")
}

TEMPLATE = lib
TARGET = core
CONFIG += staticlib
QT += serialport
QT -= gui

HEADERS += \
    portdiscovery.h \
    serialport.h \
    machineinfo.h
SOURCES += \
    serialport.cpp \
    machineinfo.cpp

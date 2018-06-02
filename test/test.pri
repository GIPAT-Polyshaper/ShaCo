# Check the config files exist
!include(../common.pri) {
    error("Couldn't find the common.pri file!")
}

TEMPLATE = app
CONFIG += testcase console
CONFIG -= app_bundle
QT += testlib serialport
QT -= gui

# NOTE: These paths are relative to the .pro file including this (which is in a subdirectory)
INCLUDEPATH += ../.. ..
LIBS += -L../../core -lcore -L../testcommon -ltestcommon

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
unix:LIBS += -L../testcommon -ltestcommon -L../../core -lcore
win32:debug:LIBS += -L../testcommon/debug -ltestcommon -L../../core/debug -lcore
win32:release:LIBS += -L../testcommon/release -ltestcommon -L../../core/release -lcore

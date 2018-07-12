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
unix:LIBS += -L../../core -lcore -L../testcommon -ltestcommon
win32:debug:LIBS += -L../../core/debug -lcore -L../testcommon/debug -ltestcommon
win32:release:LIBS += -L../../core/release -lcore -L../testcommon/release -ltestcommon

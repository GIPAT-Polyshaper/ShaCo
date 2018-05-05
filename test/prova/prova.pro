# Check the config files exist
!include(../../common.pri) {
    error("Couldn't find the common.pri file!")
}
!include(../test.pri) {
    error("Couldn't find the test.pri file!")
}

TEMPLATE = app
TARGET = prova
CONFIG += testcase
QT += testlib

SOURCES += prova_test.cpp
LIBS += -L../../core -lcore

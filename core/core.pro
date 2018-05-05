# Check if the config file exists
!include(../common.pri) {
    error("Couldn't find the common.pri file!")
}

TEMPLATE = lib
TARGET = core
CONFIG += staticlib

HEADERS += prova.h
SOURCES += prova.cpp

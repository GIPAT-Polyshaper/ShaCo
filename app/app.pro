# Check if the config file exists
!include(../common.pri) {
    error("Couldn't find the common.pri file!")
}

TEMPLATE = app
TARGET = ShaCo
QT += quick quickcontrols2 serialport
app.depends = core

SOURCES += main.cpp \
    controller.cpp \
    worker.cpp
LIBS += -L../core -lcore
RESOURCES += ../qml.qrc

HEADERS += \
    controller.h \
    worker.h

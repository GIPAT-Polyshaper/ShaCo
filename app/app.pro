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
unix:LIBS += -L../core -lcore
win32:debug:LIBS += -L../core/debug -lcore
win32:release:LIBS += -L../core/release -lcore
RESOURCES += ../qml.qrc

HEADERS += \
    controller.h \
    worker.h

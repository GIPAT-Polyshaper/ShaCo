# Check if the config file exists
!include(../common.pri) {
    error("Couldn't find the common.pri file!")
}

TEMPLATE = app
TARGET = ShaCo
QT += quick quickcontrols2 serialport
app.depends = core
macx:ICON = ../images/ShaCo.icns
win32:RC_ICONS = ../images/ShaCo.ico

HEADERS += \
    controller.h \
    worker.h \
    localshapesmodel.h
SOURCES += main.cpp \
    controller.cpp \
    worker.cpp \
    localshapesmodel.cpp

unix:LIBS += -L../core -lcore
win32:debug:LIBS += -L../core/debug -lcore
win32:release:LIBS += -L../core/release -lcore
RESOURCES += ../qml.qrc

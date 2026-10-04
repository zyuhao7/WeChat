QT += core gui widgets
CONFIG += c++17
CONFIG -= app_bundle
TARGET = ela_demo
TEMPLATE = app
DESTDIR = ./bin

# ElaWidgetTools public headers + the shared lib built with CMake
INCLUDEPATH += /tmp/ElaWidgetTools/ElaWidgetTools
LIBS += -L/tmp/elabuild/ElaWidgetTools -lElaWidgetTools
QMAKE_RPATHDIR += /tmp/elabuild/ElaWidgetTools

SOURCES += main.cpp

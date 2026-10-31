# Static build of the vendored Fluent toolkit. The app and the test binary both
# link it, so it is compiled once per build tree instead of once per target.
TEMPLATE = lib
TARGET = ElaWidgetTools
CONFIG += staticlib c++17
CONFIG -= app_bundle
QT += core gui widgets

include(ElaWidgetTools.pri)

DESTDIR = $$OUT_PWD/../lib
unix: QMAKE_CXXFLAGS += -fPIC
unix: QMAKE_CFLAGS += -fPIC

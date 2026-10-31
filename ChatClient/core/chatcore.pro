# Client sources shared by the app and the test binary. Building them into a
# static lib means tests only compile their own test_*.cpp files.
TEMPLATE = lib
TARGET = chatcore
CONFIG += staticlib c++17
CONFIG -= app_bundle
QT += core gui network widgets

CHATCLIENT_DIR = $$PWD/..
INCLUDEPATH += $$CHATCLIENT_DIR
include($$CHATCLIENT_DIR/third_party/ElaWidgetTools/ElaWidgetToolsDep.pri)

DEFINES += QT_DEPRECATED_WARNINGS

CHATCLIENT_SOURCES = $$files($$CHATCLIENT_DIR/*.cpp)
CHATCLIENT_SOURCES -= $$files($$CHATCLIENT_DIR/main.cpp)

SOURCES += $$CHATCLIENT_SOURCES
HEADERS += $$files($$CHATCLIENT_DIR/*.h)
FORMS += $$files($$CHATCLIENT_DIR/*.ui)
RESOURCES += $$CHATCLIENT_DIR/rc.qrc

DESTDIR = $$OUT_PWD/../lib
unix: QMAKE_CXXFLAGS += -fPIC
unix: QMAKE_CFLAGS += -fPIC

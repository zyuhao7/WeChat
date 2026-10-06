# Qt/Widget unit tests for the client. Build and run out of source, e.g.
#   mkdir -p build/client-tests && cd build/client-tests
#   qmake6 ../../ChatClient/tests/tests.pro && make -j"$(nproc)"
#   QT_QPA_PLATFORM=offscreen ./client_tests
QT += core gui widgets network testlib

CONFIG += c++17 testcase
CONFIG -= app_bundle

TEMPLATE = app
TARGET = client_tests

# The same Fluent toolkit the client links, compiled in as a source set.
include(../third_party/ElaWidgetTools/ElaWidgetTools.pri)

CLIENT_DIR = $$PWD/..
INCLUDEPATH += $$CLIENT_DIR

# Every client translation unit except the one holding main().
CLIENT_SOURCES = $$files($$CLIENT_DIR/*.cpp)
CLIENT_SOURCES -= $$files($$CLIENT_DIR/main.cpp)

SOURCES += test_user_list_loading.cpp \
           test_chat_dialog.cpp \
           tests_main.cpp \
           $$CLIENT_SOURCES

HEADERS += test_suites.h \
           $$files($$CLIENT_DIR/*.h)
FORMS += $$files($$CLIENT_DIR/*.ui)
RESOURCES += $$CLIENT_DIR/rc.qrc

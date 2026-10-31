# Qt/Widget unit tests for the client. Build from the shared client build tree:
#   cd ChatClient && mkdir -p build && cd build
#   qmake6 ../ChatClient.pro && make -j4
#   QT_QPA_PLATFORM=offscreen ./tests/client_tests
QT += core gui widgets network testlib

CONFIG += c++17 testcase
CONFIG -= app_bundle

TEMPLATE = app
TARGET = client_tests

CLIENT_DIR = $$PWD/..
INCLUDEPATH += $$CLIENT_DIR

# The app and the tests share these libs, so tests only compile test_*.cpp.
LIBS += -L$$OUT_PWD/../lib -lchatcore -lElaWidgetTools
include($$CLIENT_DIR/third_party/ElaWidgetTools/ElaWidgetToolsDep.pri)

SOURCES += test_usermgr.cpp \
           test_user_list_loading.cpp \
           test_chat_dialog.cpp \
           test_chat_text_msg.cpp \
           test_server_error_text.cpp \
           test_filetransfer.cpp \
           test_cropdialog.cpp \
           test_ui_wiring.cpp \
           test_chat_view.cpp \
           test_tcpmgr_reconnect.cpp \
           tests_main.cpp

HEADERS += test_suites.h

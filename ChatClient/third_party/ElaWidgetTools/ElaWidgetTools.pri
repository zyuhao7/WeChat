# ElaWidgetTools (https://github.com/Liniyous/ElaWidgetTools, MIT) integrated
# as a qmake source set. TabBar/TabWidget and the example app are pruned because
# they are the only pieces that pull in Qt private headers (qtabbar_p.h).
QT += core gui widgets
CONFIG += c++17
DEFINES += ELAWIDGETTOOLS_LIBRARY

ELAWIDGETTOOLS_ROOT = $$PWD
INCLUDEPATH += $$ELAWIDGETTOOLS_ROOT \
               $$ELAWIDGETTOOLS_ROOT/private \
               $$ELAWIDGETTOOLS_ROOT/Development \
               $$ELAWIDGETTOOLS_ROOT/Development/Command

SOURCES += $$files($$ELAWIDGETTOOLS_ROOT/*.cpp) \
           $$files($$ELAWIDGETTOOLS_ROOT/private/*.cpp) \
           $$files($$ELAWIDGETTOOLS_ROOT/Development/*.cpp) \
           $$files($$ELAWIDGETTOOLS_ROOT/Development/Command/*.cpp)

HEADERS += $$files($$ELAWIDGETTOOLS_ROOT/*.h) \
           $$files($$ELAWIDGETTOOLS_ROOT/private/*.h) \
           $$files($$ELAWIDGETTOOLS_ROOT/Development/*.h) \
           $$files($$ELAWIDGETTOOLS_ROOT/Development/Command/*.h)

RESOURCES += $$ELAWIDGETTOOLS_ROOT/ElaWidgetTools.qrc

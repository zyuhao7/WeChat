# Include paths and link flags for consumers of the ElaWidgetTools static lib.
# Kept separate from ElaWidgetTools.pri, which adds the sources themselves.
ELAWIDGETTOOLS_ROOT = $$PWD

INCLUDEPATH += $$ELAWIDGETTOOLS_ROOT \
               $$ELAWIDGETTOOLS_ROOT/private \
               $$ELAWIDGETTOOLS_ROOT/Development \
               $$ELAWIDGETTOOLS_ROOT/Development/Command

DEFINES += ELAWIDGETTOOLS_LIBRARY

# Build the client and the test binary from one shadow build:
#   cd ChatClient && mkdir -p build && cd build
#   qmake6 ../ChatClient.pro && make -j4
# Outputs: build/bin/Chat and build/tests/client_tests.
#
# The two static libs are shared by both targets, so changing a test file only
# recompiles that file instead of every client translation unit.
TEMPLATE = subdirs
CONFIG += ordered

SUBDIRS = elawidgettools chatcore app tests

elawidgettools.file = ela/ela.pro
chatcore.file = core/chatcore.pro
app.file = Chat.pro
tests.file = tests/tests.pro

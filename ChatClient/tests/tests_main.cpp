#include <QApplication>
#include <QtTest>

#include "test_suites.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    // Both resource files live in static libs; reference them so the linker
    // pulls their initializers into the test binary.
    Q_INIT_RESOURCE(rc);
    Q_INIT_RESOURCE(ElaWidgetTools);

    int status = 0;
    for (QObject* suite : {createUserMgrTest(), createUserListLoadingTest(),
                           createChatDialogTest(), createChatDialogTextMsgTest(),
                           createServerErrorTextTest(), createFileTransferHelpersTest(),
                           createCropDialogHelpersTest(), createUiWiringTest(),
                           createChatViewTest(), createTcpMgrReconnectTest()})
    {
        status |= QTest::qExec(suite, argc, argv);
        delete suite;
    }
    return status;
}

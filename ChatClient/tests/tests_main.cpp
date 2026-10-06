#include <QApplication>
#include <QtTest>

#include "test_suites.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    int status = 0;
    for (QObject *suite : {createUserListLoadingTest(), createChatDialogTest()}) {
        status |= QTest::qExec(suite, argc, argv);
        delete suite;
    }
    return status;
}

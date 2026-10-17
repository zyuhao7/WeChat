#include <QtTest>

#include "global.h"

class ServerErrorTextTest : public QObject
{
    Q_OBJECT

private slots:
    void verifyServerCodesHaveTheirOwnText()
    {
        QCOMPARE(serverErrorText(3), QStringLiteral("邮箱格式不正确"));
        QCOMPARE(serverErrorText(4), QStringLiteral("请求过于频繁，请稍后再试"));
    }

    void gateServerCodesAreDistinct()
    {
        QSet<QString> texts;
        const QList<int> codes{1001, 1002, 1003, 1004, 1005, 1006,
                               1007, 1008, 1009, 1010, 1011, 1012, 1013};
        for (int code : codes)
        {
            texts.insert(serverErrorText(code));
        }
        QCOMPARE(texts.size(), codes.size());
    }

    void unknownCodesShowTheNumber()
    {
        for (int code : {-1, 5, 999, 1014, 99999})
        {
            QVERIFY2(serverErrorText(code).contains(QString::number(code)), qPrintable(QString::number(code)));
        }
    }
};

QObject* createServerErrorTextTest()
{
    return new ServerErrorTextTest;
}

#include "test_server_error_text.moc"

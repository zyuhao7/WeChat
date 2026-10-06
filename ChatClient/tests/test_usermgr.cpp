#include <QtTest>

#include <memory>

#include "usermgr.h"

class UserMgrAccessorsTest : public QObject
{
    Q_OBJECT

private slots:
    // The singleton starts with no signed-in user. The accessors dereferenced the null
    // _user_info, so anything reading the identity before login completed crashed.
    // This slot must stay ahead of the suites that sign a user in.
    void identityAccessorsAnswerBeforeLogin()
    {
        QVERIFY(!UserMgr::GetInstance()->GetUserInfo());

        QCOMPARE(UserMgr::GetInstance()->GetUid(), 0);
        QVERIFY(UserMgr::GetInstance()->GetName().isEmpty());
        QVERIFY(UserMgr::GetInstance()->GetIcon().isEmpty());
    }

    void identityAccessorsFollowTheSignedInUser()
    {
        UserMgr::GetInstance()->SetUserInfo(
            std::make_shared<UserInfo>(9, "alice", "alice", ":/res/head.png", 0));

        QCOMPARE(UserMgr::GetInstance()->GetUid(), 9);
        QCOMPARE(UserMgr::GetInstance()->GetName(), QString("alice"));
        QCOMPARE(UserMgr::GetInstance()->GetIcon(), QString(":/res/head.png"));
    }
};

QObject *createUserMgrTest()
{
    return new UserMgrAccessorsTest;
}

#include "test_usermgr.moc"

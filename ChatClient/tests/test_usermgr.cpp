#include <QtTest>

#include <QFile>
#include <QPixmap>
#include <QStandardPaths>
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

    // The crop is stored per uid; logging in again must not fall back to the server head.
    void savedAvatarWinsOverTheServerIcon()
    {
        QStandardPaths::setTestModeEnabled(true);
        UserMgr::GetInstance()->SetUserInfo(
            std::make_shared<UserInfo>(4242, "bob", "bob", ":/res/head_1.jpg", 0));

        // nothing saved yet: the server's head path is used
        QCOMPARE(UserMgr::GetInstance()->LoadAvatar().size(), QSize(256, 256));

        QPixmap avatar(64, 64);
        avatar.fill(Qt::red);
        QVERIFY(UserMgr::GetInstance()->SaveAvatar(avatar));
        QVERIFY(UserMgr::GetInstance()->GetIcon().endsWith("4242.png"));

        const QPixmap loaded = UserMgr::GetInstance()->LoadAvatar();
        QCOMPARE(loaded.size(), QSize(64, 64));
        QCOMPARE(loaded.toImage().pixelColor(1, 1), QColor(Qt::red));

        QFile::remove(UserMgr::GetInstance()->GetIcon());
    }
};

QObject* createUserMgrTest()
{
    return new UserMgrAccessorsTest;
}

#include "test_usermgr.moc"

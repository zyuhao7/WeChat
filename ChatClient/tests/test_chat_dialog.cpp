#include <QtTest>

#include <memory>

#include "chatdialog.h"
#include "tcpmgr.h"
#include "usermgr.h"

namespace {

// ChatDialog reads the signed-in user straight out of UserMgr while building its UI.
void signInTestUser()
{
    UserMgr::GetInstance()->SetUserInfo(std::make_shared<UserInfo>(6, "tester", ""));
}

} // namespace

class ChatDialogApplyFriendTest : public QObject
{
    Q_OBJECT

private slots:
    // tcpmgr emits sig_friend_apply with a null payload when the notify-add-friend reply
    // cannot be parsed or carries an error; the slot dereferenced it straight away.
    void nullApplyNoticeIsDropped()
    {
        signInTestUser();
        ChatDialog dialog;
        const size_t applies_before = UserMgr::GetInstance()->GetApplyList().size();

        TcpMgr::GetInstance()->sig_friend_apply(nullptr);

        QCOMPARE(UserMgr::GetInstance()->GetApplyList().size(), applies_before);
    }

    void realApplyNoticeIsStillRecorded()
    {
        signInTestUser();
        ChatDialog dialog;
        const size_t applies_before = UserMgr::GetInstance()->GetApplyList().size();

        auto apply = std::make_shared<AddFriendApply>(4242, "peer", "desc", "", "nick", 1);
        TcpMgr::GetInstance()->sig_friend_apply(apply);

        QCOMPARE(UserMgr::GetInstance()->GetApplyList().size(), applies_before + 1);
        QVERIFY(UserMgr::GetInstance()->AlreadyApply(4242));
    }
};

QObject *createChatDialogTest()
{
    return new ChatDialogApplyFriendTest;
}

#include "test_chat_dialog.moc"

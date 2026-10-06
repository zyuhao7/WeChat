#include <QtTest>

#include <QJsonArray>
#include <QJsonObject>
#include <QListWidget>
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

std::shared_ptr<TextChatMsg> msgFrom(int uid)
{
    QJsonArray payload;
    QJsonObject entry;
    entry["msgid"] = "msg-1";
    entry["content"] = "hello";
    payload.append(entry);
    return std::make_shared<TextChatMsg>(uid, 6, payload);
}

} // namespace

class ChatDialogTextMsgTest : public QObject
{
    Q_OBJECT

private slots:
    // A peer missing from the friend map — a message that beats the friend-list load —
    // used to reach ChatUserWid::SetInfo as a null FriendInfo and dereference it.
    void messageFromAnUnknownPeerIsDropped()
    {
        signInTestUser();
        ChatDialog dialog;
        auto *chat_list = dialog.findChild<QListWidget *>("chat_user_list");
        QVERIFY(chat_list);
        const int items_before = chat_list->count();

        TcpMgr::GetInstance()->sig_text_chat_msg(msgFrom(31337));

        QCOMPARE(chat_list->count(), items_before);
    }

    void messageFromAKnownPeerOpensAChatItem()
    {
        signInTestUser();
        ChatDialog dialog;
        auto *chat_list = dialog.findChild<QListWidget *>("chat_user_list");
        QVERIFY(chat_list);
        const int items_before = chat_list->count();

        UserMgr::GetInstance()->AddFriend(std::make_shared<AuthInfo>(7777, "peer", "peer", "", 0));
        TcpMgr::GetInstance()->sig_text_chat_msg(msgFrom(7777));

        QCOMPARE(chat_list->count(), items_before + 1);
    }
};

QObject *createChatDialogTextMsgTest()
{
    return new ChatDialogTextMsgTest;
}

#include "test_chat_text_msg.moc"

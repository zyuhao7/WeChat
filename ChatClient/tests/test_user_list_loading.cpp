#include <QtTest>

#include <QApplication>
#include <QJsonArray>
#include <QJsonObject>
#include <QPointF>
#include <QTimer>
#include <QWheelEvent>

#include "chatuserlist.h"
#include "contactuserlist.h"
#include "global.h"
#include "usermgr.h"

namespace {

void seedFriends(int count)
{
    QJsonArray friends;
    for (int i = 0; i < count; ++i) {
        QJsonObject friend_obj;
        friend_obj["uid"] = 1000 + i;
        friend_obj["name"] = QString("friend %1").arg(i);
        friend_obj["nick"] = "nick";
        friend_obj["icon"] = "";
        friend_obj["sex"] = 0;
        friend_obj["desc"] = "";
        friend_obj["back"] = "";
        friends.append(friend_obj);
    }
    UserMgr::GetInstance()->AppendFriendList(friends);
}

void wheelAtTheBottomEdge(QWidget *viewport)
{
    const QPointF pos(10, 10);
    QWheelEvent wheel(pos, viewport->mapToGlobal(pos.toPoint()), QPoint(0, 0), QPoint(0, -120),
                      Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false);
    QCoreApplication::sendEvent(viewport, &wheel);
}

// The list releases its re-entry guard from a deferred callback. The regression was that
// the callback also called QCoreApplication::quit(), which ends the top-level event loop,
// so if it is still called the sentinel below never runs and this returns false.
bool survivesTheDeferredCallback()
{
    bool sentinel_fired = false;
    QTimer::singleShot(400, qApp, [&]() {
        sentinel_fired = true;
        qApp->quit();
    });
    qApp->exec();
    return sentinel_fired;
}

} // namespace

class UserListLoadingTest : public QObject
{
    Q_OBJECT

private slots:
    void chatListBottomScrollAsksForMoreWithoutQuitting()
    {
        seedFriends(1);

        ChatUserList list;
        QSignalSpy loading_spy(&list, &ChatUserList::sig_loading_chat_user);

        wheelAtTheBottomEdge(list.viewport());

        // emitted synchronously from the event filter, so the branch really ran
        QCOMPARE(loading_spy.count(), 1);
        QVERIFY(survivesTheDeferredCallback());
    }

    void contactListBottomScrollAsksForMoreWithoutQuitting()
    {
        // the constructor marks one page as loaded, so leave more than a page outstanding
        seedFriends(CHAT_COUNT_PER_PAGE * 2);

        ContactUserList list;
        QSignalSpy loading_spy(&list, &ContactUserList::sig_loading_contact_user);

        wheelAtTheBottomEdge(list.viewport());

        QCOMPARE(loading_spy.count(), 1);
        QVERIFY(survivesTheDeferredCallback());
    }
};

QObject *createUserListLoadingTest()
{
    return new UserListLoadingTest;
}

#include "test_user_list_loading.moc"

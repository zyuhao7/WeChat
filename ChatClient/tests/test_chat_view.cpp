#include <QtTest>

#include <QLabel>
#include <QScrollArea>
#include <QScrollBar>
#include <QVBoxLayout>

#include "chatview.h"

// The chat view decides where a reader lands when older pages are requested. If the scroll
// offset is not shifted by the inserted height the view jumps back to the newest messages,
// which makes upward paging unusable, so those two behaviours are driven directly.
class ChatViewTest : public QObject
{
    Q_OBJECT

private:
    static QWidget* MakeRow(const QString& text)
    {
        auto* row = new QLabel(text);
        row->setFixedHeight(40);
        return row;
    }

    static QScrollBar* BarOf(ChatView& view)
    {
        auto* area = view.findChild<QScrollArea*>("chat_area");
        return area ? area->verticalScrollBar() : nullptr;
    }

    static void FillWithRows(ChatView& view, int count)
    {
        for (int i = 0; i < count; ++i)
            view.appendChatItem(MakeRow(QString("row %1").arg(i)));
    }

    static void ShowAndSettle(ChatView& view)
    {
        view.resize(320, 240);
        view.show();
        QVERIFY(QTest::qWaitForWindowExposed(&view));
        // let the pending append-to-bottom timers fire before the test moves the view
        QTest::qWait(600);
    }

private slots:
    void prependedItemsLandAboveTheExistingRows()
    {
        ChatView view;
        ShowAndSettle(view);
        FillWithRows(view, 3);

        QList<QWidget*> older{MakeRow("older 0"), MakeRow("older 1")};
        view.prependChatItems(older);

        auto* background = view.findChild<QWidget*>("chat_bg");
        QVERIFY(background);
        auto* layout = qobject_cast<QVBoxLayout*>(background->layout());
        QVERIFY(layout);

        for (int i = 0; i < older.size(); ++i)
            QCOMPARE(layout->itemAt(i)->widget(), older[i]);
    }

    void emptyPrependChangesNothing()
    {
        ChatView view;
        ShowAndSettle(view);
        FillWithRows(view, 3);

        auto* background = view.findChild<QWidget*>("chat_bg");
        auto* layout = qobject_cast<QVBoxLayout*>(background->layout());
        const int before = layout->count();

        view.prependChatItems({});

        QCOMPARE(layout->count(), before);
    }

    void prependingKeepsTheReaderAtTheSameRow()
    {
        ChatView view;
        ShowAndSettle(view);
        FillWithRows(view, 40);

        QScrollBar* bar = BarOf(view);
        QVERIFY(bar);
        QTRY_VERIFY(bar->maximum() > 0);

        bar->setValue(bar->maximum());
        QCoreApplication::processEvents();
        const int old_max = bar->maximum();
        const int old_value = bar->value();
        QCOMPARE(old_value, old_max);

        QList<QWidget*> older;
        for (int i = 0; i < 5; ++i)
            older.append(MakeRow(QString("older %1").arg(i)));
        view.prependChatItems(older);

        QTRY_VERIFY(bar->maximum() > old_max);
        const int added = bar->maximum() - old_max;
        QTRY_COMPARE(bar->value(), old_value + added);
    }

    void reachedTopFiresOncePerApproach()
    {
        ChatView view;
        ShowAndSettle(view);
        FillWithRows(view, 40);

        QScrollBar* bar = BarOf(view);
        QVERIFY(bar);
        QTRY_VERIFY(bar->maximum() > 0);

        QSignalSpy spy(&view, &ChatView::sig_reached_top);

        bar->setValue(bar->maximum());
        bar->setValue(0);
        QCOMPARE(spy.count(), 1);

        // staying at the top must not keep re-triggering the request
        bar->setValue(0);
        QCOMPARE(spy.count(), 1);

        bar->setValue(10);
        bar->setValue(0);
        QCOMPARE(spy.count(), 2);
    }

    void reachedTopStaysSilentWhenEverythingFits()
    {
        ChatView view;
        ShowAndSettle(view);
        FillWithRows(view, 1);

        QScrollBar* bar = BarOf(view);
        QVERIFY(bar);
        QVERIFY(bar->maximum() == 0);

        QSignalSpy spy(&view, &ChatView::sig_reached_top);
        bar->setValue(0);
        QCOMPARE(spy.count(), 0);
    }
};

QObject* createChatViewTest()
{
    return new ChatViewTest;
}

#include "test_chat_view.moc"

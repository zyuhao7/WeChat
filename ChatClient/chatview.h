#ifndef CHATVIEW_H
#define CHATVIEW_H
#include <QScrollArea>
#include <QVBoxLayout>
#include <QTimer>
#include <QList>

class ChatView : public QWidget
{
    Q_OBJECT
public:
    ChatView(QWidget* parent = Q_NULLPTR);
    void appendChatItem(QWidget* item);                  // head insert
    void prependChatItem(QWidget* item);                 // tail insert
    void prependChatItems(const QList<QWidget*>& items); // batch insert at the top
    void insertChatItem(QWidget* before, QWidget* item); // middle insert
    void removeAllItem();

signals:
    void sig_reached_top(); // the user scrolled up to the oldest item

protected:
    bool eventFilter(QObject* o, QEvent* e) override;
    void paintEvent(QPaintEvent* event) override;

private slots:
    void onVScrollBarMoved(int min, int max);

private:
    void initStyleSheet();

private:
    QVBoxLayout* m_pVl;         // vertical layout
    QScrollArea* m_pScrollArea; // scroll area
    bool isAppended;
    bool _at_top_notified = false;

    // a prepend grows the scroll range over several layout passes; keep the reader anchored
    // until the range settles instead of shifting once from a possibly stale maximum
    bool _prepend_pending = false;
    int _prepend_anchor_max = 0;
    int _prepend_anchor_value = 0;
    QTimer* _prepend_settle_timer = nullptr;
};

#endif // CHATVIEW_H

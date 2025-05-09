#ifndef CHATVIEW_H
#define CHATVIEW_H
#include <QScrollArea>
#include <QVBoxLayout>
#include <QTimer>

class ChatView : public QWidget
{
    Q_OBJECT
public:
    ChatView(QWidget* parent = Q_NULLPTR);
    void appendChatItem(QWidget* item); // 头插
    void prependChatItem(QWidget* item); //尾插
    void insertChatItem(QWidget* before, QWidget* item); // 中间插
    void removeAllItem();
protected:
    bool eventFilter(QObject* o, QEvent* e) override;
    void paintEvent(QPaintEvent *event) override;

private slots:
    void onVScrollBarMoved(int min, int max);
private:
    void initStyleSheet();
private:
    QVBoxLayout* m_pVl;         // 垂直布局
    QScrollArea* m_pScrollArea; // 滚动区域
    bool isAppended;
};

#endif // CHATVIEW_H

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
    void appendChatItem(QWidget* item); // head insert
    void prependChatItem(QWidget* item); //tail insert
    void insertChatItem(QWidget* before, QWidget* item); // middle insert
    void removeAllItem();
protected:
    bool eventFilter(QObject* o, QEvent* e) override;
    void paintEvent(QPaintEvent *event) override;

private slots:
    void onVScrollBarMoved(int min, int max);
private:
    void initStyleSheet();
private:
    QVBoxLayout* m_pVl;         // vertical layout
    QScrollArea* m_pScrollArea; // scroll area
    bool isAppended;
};

#endif // CHATVIEW_H

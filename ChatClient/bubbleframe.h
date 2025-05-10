#ifndef BUBBLEFRAME_H
#define BUBBLEFRAME_H
#include <QFrame>
#include <QHBoxLayout>
#include "global.h"

class BubbleFrame : public QFrame // 类似于 QWidget
{
    Q_OBJECT
public:
    BubbleFrame(ChatRole role, QWidget* parent = nullptr);
    void setMargin(int margin);
    void setWidget(QWidget* w);

protected:
    void paintEvent(QPaintEvent *e);

private:
    QHBoxLayout* m_pHLayout;
    ChatRole m_role;
    int m_margin;
};

#endif // BUBBLEFRAME_H

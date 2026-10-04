#include "chatuserlist.h"
#include<QScrollBar>
#include "usermgr.h"
#include <QTimer>
#include <QCoreApplication>
#include "ElaScrollBar.h"


ChatUserList::ChatUserList(QWidget *parent)
    :QListWidget(parent),
      _load_pending(false)
{
    Q_UNUSED(parent);
    // Fluent scrollbars; the viewportEnter/Leave filter below still toggles the policy.
    this->setVerticalScrollBar(new ElaScrollBar(this));
    this->setHorizontalScrollBar(new ElaScrollBar(Qt::Horizontal, this));
    this->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    this->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // install the event filter
    this->viewport()->installEventFilter(this);
}

bool ChatUserList::eventFilter(QObject *watched, QEvent *event)
{
    // check whether the event is a mouse hover enter or leave
    if(watched == this->viewport())
    {
        if(event->type() == QEvent::Enter)
        {
            // on hover, show the scrollbar
            this->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        }
        else if(event->type() == QEvent::Leave)
        {
            // on mouse leave, hide the scrollbar
            this->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        }
    }
    // check whether the event is a mouse wheel event
    if(watched == this->viewport() && event->type() == QEvent::Wheel)
    {
        QWheelEvent* wheelEvent = static_cast<QWheelEvent*>(event);
        int numDegrees = wheelEvent->angleDelta().y() / 8;
        int numSteps = numDegrees / 15; // compute the scroll steps

        // set the scroll step
        this->verticalScrollBar()->setValue(this->verticalScrollBar()->value() - numSteps);

        // check whether scrolled to the bottom
        QScrollBar* scrollBar = this->verticalScrollBar();
        int maxScrollValue = scrollBar->maximum();
        int currentValue = scrollBar->value();

        if(maxScrollValue - currentValue <= 0)
        {
            auto b_loaded = UserMgr::GetInstance()->IsLoadChatFin();
            if(b_loaded || _load_pending)
                return true;

            // scroll to bottom to load new chat users
            qDebug() <<"Load more chat user";

            _load_pending = true;
            QTimer::singleShot(100, [this](){
                _load_pending = false;
                QCoreApplication::quit();
            });

            // emit a signal to ask the chat page to load more messages
            emit sig_loading_chat_user();
        }
        return true;
    }
    return QListWidget::eventFilter(watched, event);
}

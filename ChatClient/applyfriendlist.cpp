#include "applyfriendlist.h"
#include <QWheelEvent>
#include <QScrollBar>
#include "listitembase.h"
#include "ElaScrollBar.h"


ApplyFriendList::ApplyFriendList(QWidget *parent)
{
    Q_UNUSED(parent);
    // Fluent scrollbars; the viewportEnter/Leave filter below still toggles the policy.
    this->setVerticalScrollBar(new ElaScrollBar(this));
    this->setHorizontalScrollBar(new ElaScrollBar(Qt::Horizontal, this));
    this->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    this->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // install the event filter
    this->viewport()->installEventFilter(this);
}

bool ApplyFriendList::eventFilter(QObject *watched, QEvent *event)
{
    // check whether the event is a mouse hover enter or leave
      if (watched == this->viewport()) {
          if (event->type() == QEvent::Enter) {
              // on hover, show the scrollbar
              this->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
          } else if (event->type() == QEvent::Leave) {
              // on mouse leave, hide the scrollbar
              this->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
          }
      }

      if (watched == this->viewport()) {
          if (event->type() == QEvent::MouseButtonPress) {
              emit sig_show_search(false); // the search box is hidden
          }
     }

      // check whether the event is a mouse wheel event
      if (watched == this->viewport() && event->type() == QEvent::Wheel) {
          QWheelEvent *wheelEvent = static_cast<QWheelEvent*>(event);
          int numDegrees = wheelEvent->angleDelta().y() / 8;
          int numSteps = numDegrees / 15; // compute the scroll steps

          // set the scroll step
          this->verticalScrollBar()->setValue(this->verticalScrollBar()->value() - numSteps);

          return true; // stop event propagation
      }

      return QListWidget::eventFilter(watched, event);
}

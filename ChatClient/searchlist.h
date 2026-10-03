#ifndef SEARCHLIST_H
#define SEARCHLIST_H
#include <QListWidget>
#include <QWheelEvent>
#include <QEvent>
#include <QScrollBar>
#include <QDebug>
#include <QDialog>
#include <memory>
#include "userdata.h"
#include "loadingdlg.h"

class SearchList : public QListWidget
{
    Q_OBJECT
public:
    SearchList(QWidget *parent = nullptr);
    void CloseFindDlg();
    void SetSearchEdit(QWidget* edit);
protected:
    bool eventFilter(QObject *watched, QEvent *event) override {
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

        // check whether the event is a mouse wheel event
        if (watched == this->viewport() && event->type() == QEvent::Wheel) {
            QWheelEvent *wheelEvent = static_cast<QWheelEvent*>(event);
            int numDegrees = wheelEvent->angleDelta().y() / 8;
            int numSteps = numDegrees / 15;                     // compute the scroll steps

            // set the scroll step
            this->verticalScrollBar()->setValue(this->verticalScrollBar()->value() - numSteps);

            return true; // stop event propagation
        }

        return QListWidget::eventFilter(watched, event);
    }
private:
    void waitPending(bool pending = true);
    bool _send_pending;
    void addTipItem();
     std::shared_ptr<QDialog> _find_dlg;
    QWidget* _search_edit;
	std::shared_ptr<LoadingDlg> _loadingDialog;
private slots:
    void slot_item_clicked(QListWidgetItem *item);
    void slot_user_search(std::shared_ptr<SearchInfo> si);
signals:
    void sig_jump_chat_item(std::shared_ptr<SearchInfo> si);
};

#endif // SEARCHLIST_H

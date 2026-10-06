#include "contactuserlist.h"
#include "tcpmgr.h"
#include "global.h"
#include "listitembase.h"
#include "grouptipitem.h"
#include "conuseritem.h"
#include "usermgr.h"
#include <QRandomGenerator>
#include <QTimer>
#include "ElaScrollBar.h"

ContactUserList::ContactUserList(QWidget *parent)
    : _load_pending(false),
      _add_friend_item(nullptr)
{
    Q_UNUSED(parent);
    // Fluent scrollbars; the viewportEnter/Leave filter below still toggles the policy.
    this->setVerticalScrollBar(new ElaScrollBar(this));
    this->setHorizontalScrollBar(new ElaScrollBar(Qt::Horizontal, this));
    this->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    this->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // install the event filter
    this->viewport()->installEventFilter(this);

    // simulate data coming from the DB or backend to load the list
    addContactUserList();

    // connect the click signal and slot
    connect(this, &QListWidget::itemClicked, this, &ContactUserList::slot_item_clicked);

    // connect the peer's auth-accepted notification signal
    connect(TcpMgr::GetInstance().get(), &TcpMgr::sig_add_auth_friend, this,
            &ContactUserList::slot_add_auth_friend);

    // connect the self accept-auth UI refresh
    connect(TcpMgr::GetInstance().get(), &TcpMgr::sig_auth_rsp, this,
            &ContactUserList::slot_auth_rsp);


}

void ContactUserList::ShowRedPoint(bool bshow)
{
    _add_friend_item->ShowRedPoint(bshow);
}

bool ContactUserList::eventFilter(QObject *watched, QEvent *event)
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

        // check whether the event is a mouse wheel event
        if (watched == this->viewport() && event->type() == QEvent::Wheel) {
            QWheelEvent *wheelEvent = static_cast<QWheelEvent*>(event);
            int numDegrees = wheelEvent->angleDelta().y() / 8;
            int numSteps = numDegrees / 15; // compute the scroll steps

            // set the scroll step
            this->verticalScrollBar()->setValue(this->verticalScrollBar()->value() - numSteps);

            // check whether scrolled to the bottom
            QScrollBar *scrollBar = this->verticalScrollBar();
            int maxScrollValue = scrollBar->maximum();
            int currentValue = scrollBar->value();

            if (maxScrollValue - currentValue <= 0) {
               auto b_loaded = UserMgr::GetInstance()->IsLoadConFin();
               if(b_loaded)
                   return true;
               if(_load_pending)
                   return true;

               _load_pending = true;

               QTimer::singleShot(100, [this](){
               _load_pending = false;
               });

               // scroll to bottom to load new contacts
               qDebug()<< " Load more contact user";

                // emit a signal to ask the chat page to load more messages
                emit sig_loading_contact_user();
             }

            return true; // stop event propagation
        }

        return QListWidget::eventFilter(watched, event);
}

void ContactUserList::addContactUserList()
{
    auto* groupTip = new GroupTipItem();
    QListWidgetItem* item = new QListWidgetItem;
    item->setSizeHint(groupTip->sizeHint());
    this->addItem(item);
    this->setItemWidget(item, groupTip);
    item->setFlags(item->flags() & ~Qt::ItemIsSelectable);

    _add_friend_item = new ConUserItem();
    _add_friend_item->setObjectName("new_friend_item");
    _add_friend_item->SetInfo(0, tr("新的朋友"), ":/res/add_friend.png");
    _add_friend_item->SetItemType(ListItemType::APPLY_FRIEND_ITEM);

    QListWidgetItem* add_item = new QListWidgetItem;
    add_item->setSizeHint(_add_friend_item->sizeHint());
    this->addItem(add_item);
    this->setItemWidget(add_item, _add_friend_item);

    // default the new friend apply item to selected
    this->setCurrentItem(add_item);

    auto* groupCon = new GroupTipItem();
    groupCon->SetGroupTip(tr("联系人"));
    _groupitem = new QListWidgetItem;
    _groupitem->setSizeHint(groupCon->sizeHint());
    this->addItem(_groupitem);
    this->setItemWidget(_groupitem, groupCon);
   _groupitem->setFlags(_groupitem->flags() & ~Qt::ItemIsSelectable);

   // load the friend list sent by the backend
   auto con_list = UserMgr::GetInstance()->GetConListPerPage();
   for(auto& con_ele : con_list)
   {
       auto* con_user_wid = new ConUserItem();
       con_user_wid->SetInfo(con_ele->_uid,con_ele->_name, con_ele->_icon);
       QListWidgetItem* item = new QListWidgetItem;
       item->setSizeHint(con_user_wid->sizeHint());
       this->addItem(item);
       this->setItemWidget(item, con_user_wid);
   }

   UserMgr::GetInstance()->UpdateContactLoadedCount();

   // create a QListWidgetItem and set the custom widget
    for(int i = 0; i < 13; i++){
        int randomValue = QRandomGenerator::global()->bounded(100); // generate a random integer between 0 and 99
        int head_i = randomValue%heads.size();
        int name_i = randomValue%names.size();

        auto *con_user_wid = new ConUserItem();
        con_user_wid->SetInfo(0, names[name_i], heads[head_i]);
        QListWidgetItem *item = new QListWidgetItem;
        //qDebug()<<"chat_user_wid sizeHint is " << chat_user_wid->sizeHint();
        item->setSizeHint(con_user_wid->sizeHint());
        this->addItem(item);
        this->setItemWidget(item, con_user_wid);
    }

}

void ContactUserList::slot_item_clicked(QListWidgetItem *item)
{
       QWidget *widget = this->itemWidget(item); // get the custom widget object
       if(!widget){
           qDebug()<< "slot item clicked widget is nullptr";
           return;
       }

       // operate on the custom widget, casting the item to the base ListItemBase
       ListItemBase *customItem = qobject_cast<ListItemBase*>(widget);
       if(!customItem){
           qDebug()<< "slot item clicked widget is nullptr";
           return;
       }

       auto itemType = customItem->GetItemType();
       if(itemType == ListItemType::INVALID_ITEM
               || itemType == ListItemType::GROUP_TIP_ITEM){
           qDebug()<< "slot invalid item clicked ";
           return;
       }

      if(itemType == ListItemType::APPLY_FRIEND_ITEM){

          // create a dialog to prompt the user
          qDebug()<< "apply friend item clicked ";
          //switch to the friend apply page
          emit sig_switch_apply_friend_page();
          return;
      }

      if(itemType == ListItemType::CONTACT_USER_ITEM){
          // create a dialog to prompt the user
          qDebug()<< "contact user item clicked ";

          auto con_item = qobject_cast<ConUserItem*>(customItem);
          auto user_info = con_item->GetInfo();

          //switch to the friend info page
          emit sig_switch_friend_info_page(user_info);
          return;
      }
}

void ContactUserList::slot_add_auth_friend(std::shared_ptr<AuthInfo> auth_info)
{
    qDebug()<<"slot add auth friend";
    bool isFriend = UserMgr::GetInstance()->CheckFriendById(auth_info->_uid);
    if(isFriend)
        return;

    // insert the new item after groupItem
    auto* con_user_wid = new ConUserItem();
    con_user_wid->SetInfo(auth_info);

    QListWidgetItem* item = new QListWidgetItem;
    item->setSizeHint(con_user_wid->sizeHint());

    int index = this->row(_groupitem);
    this->insertItem(index + 1, item);
    this->setItemWidget(item, con_user_wid);
}

void ContactUserList::slot_auth_rsp(std::shared_ptr<AuthRsp> auth_rsp)
{
    qDebug()<<"slot auth rsp called";
    bool isFriend = UserMgr::GetInstance()->CheckFriendById(auth_rsp->_uid);
    if(isFriend)
        return;

    auto* con_user_wid = new ConUserItem();
    con_user_wid->SetInfo(auth_rsp);

    QListWidgetItem* item = new QListWidgetItem;
    item->setSizeHint(con_user_wid->sizeHint());

    int index = this->row(_groupitem);
    this->insertItem(index + 1, item);
    this->setItemWidget(item, con_user_wid);
}

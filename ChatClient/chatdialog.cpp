#include "chatdialog.h"
#include "ui_chatdialog.h"
#include "chatuserwid.h"
#include "loadingdlg.h"
#include "global.h"
#include "chatitembase.h"
#include "tcpmgr.h"
#include "usermgr.h"
#include "textbubble.h"
#include "picturebubble.h"
#include "messagetextedit.h"
#include "chatuserlist.h"
#include "grouptipitem.h"
#include "conuseritem.h"

#include <QAction>
#include <QMouseEvent>
#include <QRandomGenerator>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMovie>
#include <QTimer>
#include <QDebug>
#include <vector>

ChatDialog::ChatDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ChatDialog),
    _mode(ChatUIMode::ChatMode),
    _state(ChatUIMode::ChatMode),
    _b_loading(false),
    _last_widget(nullptr),
    _cur_chat_uid(0)
{
    ui->setupUi(this);
    ui->add_btn->SetState("normal", "hover", "press");
	ui->add_btn->setProperty("state", "normal");

    QAction* searchAction = new QAction(ui->search_edit);
    searchAction->setIcon(QIcon(":/res/search.png"));

    ui->search_edit->addAction(searchAction, QLineEdit::LeadingPosition);
    ui->search_edit->setPlaceholderText(QStringLiteral("搜索"));

    // create a clear action and set its icon
    QAction* clearAction = new QAction(ui->search_edit);
    clearAction->setIcon(QIcon(":/res/close_transparent.png"));

    // hide the clear icon initially
    // add the clear action to the end of the LineEdit
    ui->search_edit->addAction(clearAction, QLineEdit::TrailingPosition);

    //when the clear icon should show, switch to the real clear icon
    connect(ui->search_edit, &QLineEdit::textChanged, [clearAction](const QString& text){
        if(!text.isEmpty())
        {
            clearAction->setIcon(QIcon(":/res/close_search.png"));
        }
        else
        {
            clearAction->setIcon(QIcon(":/res/close_transparent.png")); // switch to a transparent icon when the text is empty
        }
    });

    // connect the clear action's triggered signal to the slot that clears the text
    connect(clearAction, &QAction::triggered, [this, clearAction](){
        ui->search_edit->clear();
        clearAction->setIcon(QIcon(":/res/close_transparent.png"));
        ui->search_edit->clearFocus();
        // if the clear button is pressed, hide the search box
        ShowSearch(false);
    });

    ui->search_edit->SetMaxLength(15);
    // ElaLineEdit ships its own clear button; the search box already manages a
    // trailing clear QAction, so disable the built-in one to avoid a duplicate.
    ui->search_edit->setIsClearButtonEnable(false);

     // connect the load signal and slot
    connect(ui->chat_user_list, &ChatUserList::sig_loading_chat_user, this, &ChatDialog::slot_loading_chat_user);
    addChatUserList();

     //simulate loading the own avatar
     QString head_icon = UserMgr::GetInstance()->GetIcon();
     QPixmap pixmap(head_icon);
     QPixmap scaledPixmap = pixmap.scaled( ui->side_head_lb->size(), Qt::KeepAspectRatio); // scale the image to the label size

    ui->side_head_lb->setPixmap(scaledPixmap); // set the scaled image on the QLabel
    ui->side_head_lb->setScaledContents(true); // make the QLabel scale its image to fit

    ui->side_chat_lb->setProperty("state","normal");
    ui->side_chat_lb->SetState("normal","hover","pressed","selected_normal","selected_hover","selected_pressed");
    ui->side_contact_lb->SetState("normal","hover","pressed","selected_normal","selected_hover","selected_pressed");

    AddLBGroup(ui->side_chat_lb);
    AddLBGroup(ui->side_contact_lb);

    connect(ui->side_chat_lb, &StateWidget::clicked, this, &ChatDialog::slot_side_chat);
    connect(ui->side_contact_lb, &StateWidget::clicked, this, &ChatDialog::slot_side_contact);

    //connect the search box input-changed signal
    connect(ui->search_edit, &QLineEdit::textChanged, this, &ChatDialog::slot_text_changed);

     ShowSearch(false);

    // detect the click position and decide whether to clear the search box
    this->installEventFilter(this); // install the event filter

    //set the chat label's selected state
    ui->side_chat_lb->SetSelected(true);

    // set the selected item
    SetSelectChatItem();

    //update the chat page info
    SetSelectChatPage();

    // connect the load-contacts signal and slot
    connect(ui->con_user_list, &ContactUserList::sig_loading_contact_user,
            this, &ChatDialog::slot_loading_contact_user);

    // connect the contacts page's friend-apply-item click signal
	connect(ui->con_user_list, &ContactUserList::sig_switch_apply_friend_page, this, &ChatDialog::slot_switch_apply_friend_page);

    // connect the clear-search-box action
    connect(ui->friend_apply_page, &ApplyFriendPage::sig_show_search, this, &ChatDialog::slot_show_search);

    // set search_edit for search_list
    ui->search_list->SetSearchEdit(ui->search_edit);

    // connect the add-friend-apply signal
    connect(TcpMgr::GetInstance().get(), &TcpMgr::sig_friend_apply, this, &ChatDialog::slot_apply_friend);

    // connect the auth add-friend info
    connect(TcpMgr::GetInstance().get(), &TcpMgr::sig_add_auth_friend, this, &ChatDialog::slot_add_auth_friend);

    // connect the self auth-reply signal
    connect(TcpMgr::GetInstance().get(), &TcpMgr::sig_auth_rsp, this, &ChatDialog::slot_auth_rsp);

    // connect the contact-item click signal to the user-info display slot
    connect(ui->con_user_list, &ContactUserList::sig_switch_friend_info_page, this, &ChatDialog::slot_friend_info_page);

    // set the central widget to chatpage
    ui->stackedWidget->setCurrentWidget(ui->chat_page);

    //connect the searchlist jump-to-chat signal
       connect(ui->search_list, &SearchList::sig_jump_chat_item, this, &ChatDialog::slot_jump_chat_item);

   //connect the click event from the friend info page
   connect(ui->friend_info_page, &FriendInfoPage::sig_jump_chat_item, this,
           &ChatDialog::slot_jump_chat_item_from_infopage);

   //connect the chat list click signal
   connect(ui->chat_user_list, &QListWidget::itemClicked, this, &ChatDialog::slot_item_clicked);

   //connect the peer message notification
   connect(TcpMgr::GetInstance().get(), &TcpMgr::sig_text_chat_msg,
           this, &ChatDialog::slot_text_chat_msg);

   connect(ui->chat_page, &ChatPage::sig_append_send_chat_msg, this, &ChatDialog::slot_append_send_chat_msg);

   _timer = new QTimer(this);
   connect(_timer, &QTimer::timeout, this, [](){
       auto user_info = UserMgr::GetInstance()->GetUserInfo();
       QJsonObject textobj;
       textobj["fromuid"] = user_info->_uid;
       QJsonDocument doc(textobj);
       QByteArray jsonData = doc.toJson(QJsonDocument::Compact);

       emit TcpMgr::GetInstance()->sig_send_data(ReqId::ID_HEART_BEAT_REQ, jsonData);
   });

   _timer->start(10000);
}

ChatDialog::~ChatDialog()
{
    _timer->stop();
    delete ui;
}

void ChatDialog::addChatUserList()
{
    // load chat history by friend list for now; sort by last message once the client has a chat-history DB
    auto friend_list = UserMgr::GetInstance()->GetChatListPerPage();
    if(!friend_list.empty())
    {
        for(auto& e : friend_list)
        {
            auto it = _chat_items_added.find(e->_uid);
            if(it != _chat_items_added.end())
                continue;

            auto* chat_user_wid = new ChatUserWid();
            auto user_info = std::make_shared<UserInfo>(e);
            chat_user_wid->SetInfo(user_info);

            QListWidgetItem* item = new QListWidgetItem;
            item->setSizeHint(chat_user_wid->sizeHint());
            ui->chat_user_list->addItem(item);
            ui->chat_user_list->setItemWidget(item, chat_user_wid);
            _chat_items_added.insert(e->_uid, item);
        }
        // update the loaded items
        UserMgr::GetInstance()->UpdateChatLoadedCount();
    }


    // simulated test items
    // create a QListWidgetItem and set the custom widget
    for(int i = 0; i < 13; ++i)
    {
        int randomValue = QRandomGenerator::global()->bounded(100); // generate a random integer between 0 and 99
        int str_i = randomValue % strs.size();
        int head_i = randomValue % heads.size();
        int name_i = randomValue % names.size();

        auto* chat_user_wid = new ChatUserWid();
        auto user_info = std::make_shared<UserInfo>(0, names[name_i],names[name_i], heads[head_i],0, strs[str_i]);

        chat_user_wid->SetInfo(user_info);
        QListWidgetItem* item = new QListWidgetItem;
        item->setSizeHint(chat_user_wid->sizeHint());
        ui->chat_user_list->addItem(item);
        ui->chat_user_list->setItemWidget(item, chat_user_wid);
    }
}

void ChatDialog::loadMoreChatUser()
{
    auto friend_list = UserMgr::GetInstance()->GetChatListPerPage();
    if(!friend_list.empty())
    {
        for(auto& e : friend_list)
        {
            auto it = _chat_items_added.find(e->_uid);
            if(it != _chat_items_added.end())
                continue;

            auto* chat_user_wid = new ChatUserWid();
            auto user_info = std::make_shared<UserInfo>(e);
            chat_user_wid->SetInfo(user_info);

            QListWidgetItem* item = new QListWidgetItem;
            item->setSizeHint(chat_user_wid->sizeHint());
            ui->chat_user_list->addItem(item);
            ui->chat_user_list->setItemWidget(item, chat_user_wid);
            _chat_items_added.insert(e->_uid, item);
        }
        // update the loaded items
        UserMgr::GetInstance()->UpdateChatLoadedCount();
    }
}

void ChatDialog::loadMoreConUser()
{
    auto friend_list = UserMgr::GetInstance()->GetConListPerPage();
    if(!friend_list.empty())
    {
        for(auto& e : friend_list)
        {
            auto* con_user_wid = new ConUserItem();
            con_user_wid->SetInfo(e->_uid, e->_name, e->_icon);

            QListWidgetItem* item = new QListWidgetItem;
            item->setSizeHint(con_user_wid->sizeHint());
            ui->con_user_list->addItem(item);
            ui->con_user_list->setItemWidget(item, con_user_wid);
        }

        UserMgr::GetInstance()->UpdateContactLoadedCount();
    }
}

void ChatDialog::ClearLabelState(StateWidget *lb)
{
    for(auto & ele: _lb_list){
        if(ele == lb){
            continue;
        }

        ele->ClearState();
    }

}

void ChatDialog::SetSelectChatItem(int uid)
{
    if(ui->chat_user_list->count() <= 0)
    {
        return;
    }
    if(uid == 0)
    {
        ui->chat_user_list->setCurrentRow(0);
        QListWidgetItem* firstItem = ui->chat_user_list->item(0);
        if(!firstItem)
            return;
        // cast to Widget
        QWidget* widget = ui->chat_user_list->itemWidget(firstItem);
        if(!widget)
            return;

        auto chat_item = qobject_cast<ChatUserWid*>(widget);
        if(!chat_item)
            return;
        _cur_chat_uid = chat_item->GetUserInfo()->_uid;
        return;
    }
    auto it = _chat_items_added.find(uid);
    if(it == _chat_items_added.end())
    {
        qDebug()<<"uid "<<uid<<"not found, set current row 0";
        ui->chat_user_list->setCurrentRow(0);
        return;
    }
    ui->chat_user_list->setCurrentItem(it.value());
    _cur_chat_uid = uid;
}

void ChatDialog::SetSelectChatPage(int uid)
{
    // the chat list is empty
    if( ui->chat_user_list->count() <= 0)
    {
            return;
    }

    if (uid == 0) {
       auto item = ui->chat_user_list->item(0);
       //cast to widget
       QWidget* widget = ui->chat_user_list->itemWidget(item);
       if (!widget) {
           return;
       }

       auto chat_item = qobject_cast<ChatUserWid*>(widget);
       if (!chat_item) {
           return;
       }

       //set the info
       auto user_info = chat_item->GetUserInfo();
       ui->chat_page->SetUserInfo(user_info);
       return;
    }

    auto find_iter = _chat_items_added.find(uid);
    if(find_iter == _chat_items_added.end()){
        return;
    }

    //cast to widget
    QWidget *widget = ui->chat_user_list->itemWidget(find_iter.value());
    if(!widget){
        return;
    }

    //check and cast to the custom widget
    // operate on the custom widget, casting the item to the base ListItemBase
    ListItemBase *customItem = qobject_cast<ListItemBase*>(widget);
    if(!customItem){
        qDebug()<< "qobject_cast<ListItemBase*>(widget) is nullptr";
        return;
    }

    auto itemType = customItem->GetItemType();
    if(itemType == CHAT_USER_ITEM){
        auto chat_item = qobject_cast<ChatUserWid*>(customItem);
        if(!chat_item){
            return;
        }

        //set the info
        auto user_info = chat_item->GetUserInfo();
       ui->chat_page->SetUserInfo(user_info);

        return;
    }
}

bool ChatDialog::eventFilter(QObject *watched, QEvent *event)
{
    if(event->type() == QEvent::MouseButtonPress)
    {
        QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
        handleGlobalMousePress(mouseEvent);
    }
    return QDialog::eventFilter(watched, event);
}

void ChatDialog::handleGlobalMousePress(QMouseEvent *event)
{
      // implement the click-position check and handling
       // check whether we are in search mode; if not, return immediately
       if( _mode != ChatUIMode::SearchMode){
           return;
       }

       // convert the mouse click to the search list's coordinate system
       QPoint posInSearchList =(ui->search_list->mapFromGlobal(event->globalPosition())).toPoint();
       // check whether the click is within the chat list bounds
       if (!ui->search_list->rect().contains(posInSearchList)) {
           // if not in the chat list, clear the input box
           ui->search_edit->clear();
           ShowSearch(false);
       }
}

void ChatDialog::CloseFindDlg()
{
    ui->search_list->CloseFindDlg();
}

void ChatDialog::UpdateChatMsg(std::vector<std::shared_ptr<TextChatData> > msgdata)
{
    for(auto& msg : msgdata)
    {
        if(msg->_from_uid != _cur_chat_uid)
            break;
        ui->chat_page->AppendChatMsg(msg);
    }

}


void ChatDialog::ShowSearch(bool bsearch)
{
    if(bsearch)
    {
        ui->chat_user_list->hide();
        ui->con_user_list->hide();
        ui->search_list->show();
        _mode = ChatUIMode::SearchMode;
    }
    else if(_state == ChatUIMode::ChatMode)
    {
        ui->chat_user_list->show();
        ui->con_user_list->hide();
        ui->search_list->hide();
        _mode = ChatUIMode::ChatMode;
        ui->search_list->CloseFindDlg();
        ui->search_edit->clear();
        ui->search_edit->clearFocus();
    }
    else if(_state == ChatUIMode::ContactMode)
    {
        ui->chat_user_list->hide();
        ui->con_user_list->show();
        ui->search_list->hide();
        _mode = ChatUIMode::ContactMode;
        ui->search_list->CloseFindDlg();
        ui->search_edit->clear();
        ui->search_edit->clearFocus();
    }
}

void ChatDialog::AddLBGroup(StateWidget *lb)
{
    _lb_list.push_back(lb);
}

void ChatDialog::slot_loading_chat_user()
{
    if(_b_loading)
        return;
    _b_loading = true;
    LoadingDlg *loadingDialog = new LoadingDlg(this);
    loadingDialog->setModal(true);
    loadingDialog->show();
    qDebug() << "add new data to list.....";
    loadMoreChatUser();
    // close the dialog when loading completes
    loadingDialog->deleteLater();

    _b_loading = false;


    // QLabel* loading_item = new QLabel(this);
    // QMovie* movie = new QMovie(":/res/loading.gif");

    // loading_item->setMovie(movie);
    // loading_item->setFixedSize(250,70);
    // loading_item->setAlignment(Qt::AlignCenter);
    // movie->setScaledSize(QSize(50,50)); // set a fixed size of 50x50

    // QListWidgetItem* item = new QListWidgetItem;
    // item->setSizeHint(QSize(250,70));

    // ui->chat_user_list->addItem(item);
    // ui->chat_user_list->setItemWidget(item, loading_item);
    // movie->start();

    //  QTimer::singleShot(1000, this, [this, item](){
    //    qDebug()<<"add new Data to list...";
    //    addChatUserList();
    //    ui->chat_user_list->takeItem(ui->chat_user_list->row(item));
    //    ui->chat_user_list->update();
    //    _b_loading = false;
    // });
}

void ChatDialog::slot_loading_contact_user()
{
    qDebug() << "slot loading contact user";
    if(_b_loading){
        return;
    }

    _b_loading = true;
    LoadingDlg *loadingDialog = new LoadingDlg(this);
    loadingDialog->setModal(true);
    loadingDialog->show();

    qDebug() << "add new data to list.....";
    loadMoreConUser();
    // close the dialog when loading completes
    loadingDialog->deleteLater();

    _b_loading = false;
}

void ChatDialog::slot_side_chat()
{
       qDebug()<< "receive side chat clicked";
       ClearLabelState(ui->side_chat_lb);
       ui->stackedWidget->setCurrentWidget(ui->chat_page);
       _state = ChatUIMode::ChatMode;
       ShowSearch(false);
}

void ChatDialog::slot_side_contact()
{
    qDebug()<<"receive side contact clicked";
    ClearLabelState(ui->side_contact_lb);
    //set
    if(_last_widget == nullptr)
    {
        ui->stackedWidget->setCurrentWidget(ui->friend_apply_page);
        _last_widget = ui->friend_apply_page;
    }
    else
    {
        ui->stackedWidget->setCurrentWidget(_last_widget);
    }

    _state = ChatUIMode::ContactMode;
    ShowSearch(false);
}

void ChatDialog::slot_show_search(bool show)
{
    ShowSearch(show);
}

void ChatDialog::slot_text_changed(const QString &str)
{
    qDebug() << "receive slot text changed str is "<<str;
    if(!str.isEmpty())
        ShowSearch(true);
}

void ChatDialog::slot_focus_out()
{
    qDebug()<< "receive focus out signal";
    ShowSearch(false);
}

void ChatDialog::slot_switch_apply_friend_page()
{
    qDebug()<<"receive switch apply frien page sig";
    _last_widget = ui->friend_apply_page;
    ui->stackedWidget->setCurrentWidget(ui->friend_apply_page);
}

void ChatDialog::slot_apply_friend(std::shared_ptr<AddFriendApply> apply)
{
    qDebug() <<"receive apply friend slot, applyuid is "<< apply->_from_uid <<" name is "<<apply->_name <<" desc is "<<apply->_desc;
    bool b_already = UserMgr::GetInstance()->AlreadyApply(apply->_from_uid);
    if(b_already) return;

    UserMgr::GetInstance()->AddApplyList(std::make_shared<ApplyInfo>(apply));
    ui->side_contact_lb->ShowRedPoint(true);
    ui->con_user_list->ShowRedPoint(true);
    ui->friend_apply_page->AddNewApply(apply);
}

void ChatDialog::slot_add_auth_friend(std::shared_ptr<AuthInfo> auth_info)
{
    qDebug()<<"receive slot_add_auth_friend uid is"<<auth_info->_uid
           <<"name is "<<auth_info->_name<<" nick is "<<auth_info->_nick;

    // skip if already a friend
    auto bfriend = UserMgr::GetInstance()->CheckFriendById(auth_info->_uid);
    if(bfriend)
        return;

    UserMgr::GetInstance()->AddFriend(auth_info);

    auto* chat_user_wid = new ChatUserWid();
    auto  user_info = std::make_shared<UserInfo>(auth_info);
    chat_user_wid->SetInfo(user_info);

    QListWidgetItem* item = new QListWidgetItem;
    item->setSizeHint(chat_user_wid->sizeHint());
    ui->chat_user_list->insertItem(0, item);
    ui->chat_user_list->setItemWidget(item, chat_user_wid);
    _chat_items_added.insert(auth_info->_uid, item);
}

void ChatDialog::slot_auth_rsp(std::shared_ptr<AuthRsp> auth_rsp)
{
    qDebug() << "receive slot_auth_rsp uid is " << auth_rsp->_uid
            << " name is " << auth_rsp->_name << " nick is " << auth_rsp->_nick;

        //skip if already a friend
        auto bfriend = UserMgr::GetInstance()->CheckFriendById(auth_rsp->_uid);
        if(bfriend){
            return;
        }

        UserMgr::GetInstance()->AddFriend(auth_rsp);

        auto* chat_user_wid = new ChatUserWid();
        auto user_info = std::make_shared<UserInfo>(auth_rsp);
        chat_user_wid->SetInfo(user_info);
        QListWidgetItem* item = new QListWidgetItem;

        item->setSizeHint(chat_user_wid->sizeHint());
        ui->chat_user_list->insertItem(0, item);
        ui->chat_user_list->setItemWidget(item, chat_user_wid);
        _chat_items_added.insert(auth_rsp->_uid, item);
}

void ChatDialog::slot_jump_chat_item(std::shared_ptr<SearchInfo> si)
{
    qDebug()<<"slot jump chat item"<< Qt::endl;
    auto it = _chat_items_added.find(si->_uid);
    if(it != _chat_items_added.end())
    {
        qDebug()<<"jump to chat item, uid is "<<si->_uid;
        ui->chat_user_list->scrollToItem(it.value());
        ui->side_chat_lb->SetSelected(true);
        SetSelectChatItem(si->_uid);
        SetSelectChatPage(si->_uid);
        slot_side_chat();
        return;
    }

    auto* chat_user_wid = new ChatUserWid();
    auto user_info = std::make_shared<UserInfo>(si);
    chat_user_wid->SetInfo(user_info);

    QListWidgetItem* item = new QListWidgetItem;
    item->setSizeHint(chat_user_wid->sizeHint());
    ui->chat_user_list->insertItem(0, item);
    ui->chat_user_list->setItemWidget(item, chat_user_wid);

    _chat_items_added.insert(si->_uid, item);

    ui->side_chat_lb->SetSelected(true);
    SetSelectChatItem(si->_uid);
    SetSelectChatPage(si->_uid);
    slot_side_chat();
}

void ChatDialog::slot_jump_chat_item_from_infopage(std::shared_ptr<UserInfo> user_info)
{
    qDebug()<<"slot jump chat item"<< Qt::endl;
    auto it = _chat_items_added.find(user_info->_uid);
    if(it != _chat_items_added.end())
    {
        qDebug()<<"jump to chat item, uid is "<<user_info->_uid;
        ui->chat_user_list->scrollToItem(it.value());
        ui->side_chat_lb->SetSelected(true);
        SetSelectChatItem(user_info->_uid);
        SetSelectChatPage(user_info->_uid);
        slot_side_chat();
        return;
    }

    auto* chat_user_wid = new ChatUserWid();
    chat_user_wid->SetInfo(user_info);

    QListWidgetItem* item = new QListWidgetItem;
    item->setSizeHint(chat_user_wid->sizeHint());
    ui->chat_user_list->insertItem(0, item);
    ui->chat_user_list->setItemWidget(item, chat_user_wid);

    _chat_items_added.insert(user_info->_uid, item);

    ui->side_chat_lb->SetSelected(true);
    SetSelectChatItem(user_info->_uid);
    SetSelectChatPage(user_info->_uid);
    slot_side_chat();
}

void ChatDialog::slot_friend_info_page(std::shared_ptr<UserInfo> user_info)
{
    qDebug()<<"receive switch friend info page sig";
    _last_widget = ui->friend_info_page;
    ui->stackedWidget->setCurrentWidget(ui->friend_info_page);
    ui->friend_info_page->SetInfo(user_info);
}

void ChatDialog::slot_item_clicked(QListWidgetItem *item)
{
    QWidget* widget = ui->chat_user_list->itemWidget(item);
    if(!widget)
    {
        qDebug()<<"slot item clicked widget is nullptr";
        return;
    }
    ListItemBase* customItem = qobject_cast<ListItemBase*>(widget);
    if(!customItem)
    {
        qDebug()<<"slot item clicked widget is nullptr";
        return;
    }
    auto itemType = customItem->GetItemType();
    if(itemType == ListItemType::INVALID_ITEM ||
            itemType == ListItemType::GROUP_TIP_ITEM)
    {
        qDebug()<<" slot invalid item clicked";
        return;
    }
    if(itemType == ListItemType::CHAT_USER_ITEM)
    {
        //create the dialog
        qDebug()<<"contact user item clicked";
        auto chat_wid = qobject_cast<ChatUserWid*>(customItem);
        auto user_info = chat_wid->GetUserInfo();

        // // switch to the chat page
        ui->chat_page->SetUserInfo(user_info);
        _cur_chat_uid = user_info->_uid;
        return;
    }
}

void ChatDialog::slot_text_chat_msg(std::shared_ptr<TextChatMsg> msg)
{
    auto it = _chat_items_added.find(msg->_from_uid);
    if(it != _chat_items_added.end())
    {
       qDebug()<<"set chat item msg, uid is " << msg->_from_uid;
       QWidget* widget = ui->chat_user_list->itemWidget(it.value());
       auto chat_wid = qobject_cast<ChatUserWid*>(widget);
       if(!chat_wid)
           return;
       chat_wid->UpdateLastMsg(msg->_chat_msgs);
       // update the current chat page record
       UpdateChatMsg(msg->_chat_msgs);
       UserMgr::GetInstance()->AppendFriendChatMsg(msg->_from_uid, msg->_chat_msgs);
       // + a return, either omit else or write else.
    }
    else
    {
        // if not found, create and insert a new ListWidget
        auto* chat_user_wid = new ChatUserWid();
        auto fri_ptr = UserMgr::GetInstance()->GetFriendById(msg->_from_uid);
        chat_user_wid->SetInfo(fri_ptr);

        QListWidgetItem* item = new QListWidgetItem;
        item->setSizeHint(chat_user_wid->sizeHint());
        chat_user_wid->UpdateLastMsg(msg->_chat_msgs);
        UserMgr::GetInstance()->AppendFriendChatMsg(msg->_from_uid, msg->_chat_msgs);
        ui->chat_user_list->insertItem(0, item);
        ui->chat_user_list->setItemWidget(item, chat_user_wid);
        _chat_items_added.insert(msg->_from_uid, item);
    }


}

void ChatDialog::slot_append_send_chat_msg(std::shared_ptr<TextChatData> msg_data)
{
    if(_cur_chat_uid == 0)
        return ;
    auto it = _chat_items_added.find(_cur_chat_uid);
    if(it == _chat_items_added.end())
        return;
    QWidget* widget = ui->chat_user_list->itemWidget(it.value());
    if(!widget)
        return;

    ListItemBase* customItem = qobject_cast<ListItemBase*>(widget);
    if(!customItem)
    {
       qDebug()<<"qobject_cast<ListItemBase*>(widget) is nullptr";
       return;
    }

    auto itemType = customItem->GetItemType();
    if(itemType == CHAT_USER_ITEM)
    {
        auto chat_item = qobject_cast<ChatUserWid*>(customItem);
        if(!chat_item)
            return;
        auto user_info = chat_item->GetUserInfo();
        user_info->_chat_msgs.push_back(msg_data);
        std::vector<std::shared_ptr<TextChatData>> msg_vec;
        msg_vec.push_back(msg_data);
        UserMgr::GetInstance()->AppendFriendChatMsg(_cur_chat_uid, msg_vec);
        return;
    }
}




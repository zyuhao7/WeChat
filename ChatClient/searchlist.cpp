#include "searchlist.h"
#include "tcpmgr.h"
#include "loadingdlg.h"
#include "customizeedit.h"
#include "adduseritem.h"
#include "userdata.h"
#include "findsuccessdlg.h"
#include "usermgr.h"
#include <QJsonDocument>
#include "findfaildlg.h"
#include <QScrollBar>
#include "ElaScrollBar.h"

SearchList::SearchList(QWidget *parent)
	:QListWidget(parent), _send_pending(false), _find_dlg(nullptr), _search_edit(nullptr), _loadingDialog(nullptr)
{
    Q_UNUSED(parent);
     // Fluent scrollbars; the viewportEnter/Leave filter below still toggles the policy.
     this->setVerticalScrollBar(new ElaScrollBar(this));
     this->setHorizontalScrollBar(new ElaScrollBar(Qt::Horizontal, this));
     this->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
     this->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    // install the event filter
    this->viewport()->installEventFilter(this);

    //connect the click signal and slot
    connect(this, &QListWidget::itemClicked, this, &SearchList::slot_item_clicked);
    //add the item
    addTipItem();
    //connect the search item
    connect(TcpMgr::GetInstance().get(), &TcpMgr::sig_user_search, this, &SearchList::slot_user_search);
}

void SearchList::CloseFindDlg()
{
    if(_find_dlg)
    {
        _find_dlg->hide();
        _find_dlg = nullptr;
    }
}

void SearchList::SetSearchEdit(QWidget *edit)
{
    _search_edit = edit;
}

void SearchList::waitPending(bool pending)
{
	if (pending)
	{
		if (_loadingDialog == nullptr)
		{
			_loadingDialog = std::make_shared<LoadingDlg>(this);
			_loadingDialog->setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
			_loadingDialog->setAttribute(Qt::WA_TranslucentBackground, true);
			_loadingDialog->setModal(true);
		}
		_loadingDialog->show();
	}
	else
	{
		if (_loadingDialog)
		{
            _loadingDialog->hide();
			_loadingDialog = nullptr;
		}
	}
    _send_pending = pending;
}

void SearchList::addTipItem()
{
       auto *invalid_item = new QWidget();
       QListWidgetItem *item_tmp = new QListWidgetItem;
       //qDebug()<<"chat_user_wid sizeHint is " << chat_user_wid->sizeHint();
       item_tmp->setSizeHint(QSize(250,10));
       this->addItem(item_tmp);
       invalid_item->setObjectName("invalid_item");
       this->setItemWidget(item_tmp, invalid_item);
       item_tmp->setFlags(item_tmp->flags() & ~Qt::ItemIsSelectable);


       auto *add_user_item = new AddUserItem();
       QListWidgetItem *item = new QListWidgetItem;
       //qDebug()<<"chat_user_wid sizeHint is " << chat_user_wid->sizeHint();
       item->setSizeHint(add_user_item->sizeHint());
       this->addItem(item);
       this->setItemWidget(item, add_user_item);
}

void SearchList::slot_item_clicked(QListWidgetItem *item)
{
    QWidget *widget = this->itemWidget(item); //get the custom widget object
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
    if(itemType == ListItemType::INVALID_ITEM){
        qDebug()<< "slot invalid item clicked ";
        return;
    }
    if(itemType == ListItemType::ADD_USER_TIP_ITEM){
        if(_send_pending)
            return;
        if(!_search_edit){
            return;
        }

        waitPending(true);
        auto search_edit = dynamic_cast<CustomizeEdit*>(_search_edit);
        auto uid_str = search_edit->text();
        QJsonObject jsonObj;
        jsonObj["uid"] = uid_str;

        QJsonDocument  doc(jsonObj);
        QByteArray jsonData = doc.toJson(QJsonDocument::Compact);

        emit TcpMgr::GetInstance()->sig_send_data(ReqId::ID_SEARCH_USER_REQ, jsonData);
        CloseFindDlg();
        return;
    }

    //clear the popup
    CloseFindDlg();
}

void SearchList::slot_user_search(std::shared_ptr<SearchInfo> si)
{
    waitPending(false);
    if (si == nullptr) {
        _find_dlg = std::make_shared<FindFailDlg>(this);
    }
    else {
        //if it is self, return directly for now; revisit as the logic grows
        auto self_uid = UserMgr::GetInstance()->GetUid();
        if (si->_uid == self_uid) {
            return;
        }
        //two cases here: the searched user is already a friend, or not yet added
        //check whether already a friend
        bool bExist = UserMgr::GetInstance()->CheckFriendById(si->_uid);
        if (bExist) {
            //handle an already-added friend here and navigate the page
        //switch to the given item in the chat page
            emit sig_jump_chat_item(si);
            return;
        }
        //here treat it as an added friend for now
        _find_dlg = std::make_shared<FindSuccessDlg>(this);
        std::dynamic_pointer_cast<FindSuccessDlg>(_find_dlg)->SetSearchInfo(si);

    }
    _find_dlg->show();
}


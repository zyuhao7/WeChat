#include "chatpage.h"
#include "ui_chatpage.h"
#include "chatitembase.h"
#include <QStyleOption>
#include <QPainter>
#include "chatitembase.h"
#include "textbubble.h"
#include "picturebubble.h"
#include "messagetextedit.h"
#include "usermgr.h"
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include "tcpmgr.h"
#include <QUuid>

ChatPage::ChatPage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ChatPage)
{
    ui->setupUi(this);
    // set the button style
    ui->receive_btn->SetState("normal", "hover","press");
    ui->send_btn->SetState("normal", "hover", "press");

    // set the icon style
    ui->emo_lb->SetState("normal", "hover", "press", "normal", "hover", "press");
    ui->file_lb->SetState("normal", "hover", "press", "normal", "hover", "press");

    // add the Enter-to-send-message signal
    connect(ui->chatEdit, &MessageTextEdit::send, this, &ChatPage::on_send_btn_clicked);
}

ChatPage::~ChatPage()
{
    delete ui;
}

void ChatPage::SetUserInfo(std::shared_ptr<UserInfo> user_info)
{
    _user_info = user_info;
    // set the UI display
    ui->title_lb->setText(_user_info->_name);
    ui->chat_data_list->removeAllItem();
    for(auto& msg : user_info->_chat_msgs)
    {
        AppendChatMsg(msg);
    }
}

void ChatPage::AppendChatMsg(std::shared_ptr<TextChatData> msg)
{
    auto self_info = UserMgr::GetInstance()->GetUserInfo();
    ChatRole role;
    if(msg->_from_uid == self_info->_uid)
    {
        role = ChatRole::Self;
        ChatItemBase* pChatItem = new ChatItemBase(role);

        pChatItem->setUserName(self_info->_name);
        pChatItem->setUserIcon(QPixmap(self_info->_icon));

        QWidget* pBubble = nullptr;
        pBubble = new TextBubble(role, msg->_msg_content);
        pChatItem->setWidget(pBubble);
        ui->chat_data_list->appendChatItem(pChatItem);
    }
    else
    {
        role = ChatRole::Other;
        ChatItemBase* pChatItem = new ChatItemBase(role);
        auto friend_info = UserMgr::GetInstance()->GetFriendById(msg->_from_uid);
        if(friend_info == nullptr)
        {
            delete pChatItem;
            return;
        }

        pChatItem->setUserName(friend_info->_name);
        pChatItem->setUserIcon(QPixmap(friend_info->_icon));

        QWidget* pBubble = nullptr;
        pBubble  = new TextBubble(role, msg->_msg_content);
        pChatItem->setWidget(pBubble);
        ui->chat_data_list->appendChatItem(pChatItem);
    }
}

void ChatPage::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QStyleOption opt;
    opt.initFrom(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);

}

void ChatPage::on_send_btn_clicked()
{
       if(_user_info == nullptr)
       {
           qDebug()<<"friend_info is empty";
           return;
       }

       auto user_info = UserMgr::GetInstance()->GetUserInfo();
       auto pTextEdit = ui->chatEdit;
       ChatRole role = ChatRole::Self;
       QString userName = user_info->_name;
       QString userIcon = user_info->_icon;

       const QVector<MsgInfo>& msgList = pTextEdit->getMsgList();
       QJsonObject textObj;
       QJsonArray textArray;
       int txt_size = 0;
       for(int i=0; i<msgList.size(); ++i)
       {
           //skip if the message body exceeds the max length
           if(msgList[i].content.length() > 1024)
               continue;

           QString type = msgList[i].msgFlag;
           ChatItemBase *pChatItem = new ChatItemBase(role);

           pChatItem->setUserName(userName);
           pChatItem->setUserIcon(QPixmap(userIcon));

           QWidget *pBubble = nullptr;
           if(type == "text")
           {
               QUuid uuid = QUuid::createUuid();
               QString uuidString = uuid.toString();

               pBubble = new TextBubble(role, msgList[i].content);
               // send the accumulated messages and reset.
               if(txt_size + msgList[i].content.length() > 1024)
               {
                   textObj["fromuid"] = user_info->_uid;
                   textObj["touid"] = _user_info->_uid;
                   textObj["text_array"] = textArray;
                   QJsonDocument doc(textObj);
                   QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
                   // // send and clear the previously accumulated text list
                   txt_size = 0;
                   textArray = QJsonArray();
                   textObj = QJsonObject();

                   emit TcpMgr::GetInstance()->sig_send_data(ReqId::ID_TEXT_CHAT_MSG_REQ, jsonData);
               }

               //bind the bubble to the uid so delivery can be marked once the network replies
               // _bubble_map[uuidString] = pBubble;
               txt_size += msgList[i].content.length();
               QJsonObject obj;
               QByteArray utf8Message = msgList[i].content.toUtf8();
               obj["content"] = QString::fromUtf8(utf8Message);
               obj["msgid"] = uuidString;
               textArray.append(obj);

               // trigger a signal to update the local chat history
               auto txt_msg = std::make_shared<TextChatData>(uuidString, obj["content"].toString(),
                   user_info->_uid, _user_info->_uid);

               emit sig_append_send_chat_msg(txt_msg);
           }
           else if(type == "image")
           {
               pBubble = new PictureBubble(QPixmap(msgList[i].content) , role);
           }
           else if(type == "file")
           {

           }
           // :) add the bubble to the chat page
           if(pBubble != nullptr)
           {
               pChatItem->setWidget(pBubble);
               ui->chat_data_list->appendChatItem(pChatItem);
           }
       }

       qDebug() << "textArray is " << textArray;
      //send to the server
      textObj["text_array"] = textArray;
      textObj["fromuid"] = user_info->_uid;
      textObj["touid"] = _user_info->_uid;
      QJsonDocument doc(textObj);
      QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
      //send and clear the previously accumulated text list
      txt_size = 0;
      textArray = QJsonArray();
      textObj = QJsonObject();
      //send the TCP request to the chat server
      emit TcpMgr::GetInstance()->sig_send_data(ReqId::ID_TEXT_CHAT_MSG_REQ, jsonData);
}

void ChatPage::on_receive_btn_clicked()
{
    auto pTextEdit = ui->chatEdit;
    ChatRole role = ChatRole::Other;
    QString userName = _user_info->_name;
    QString userIcon = _user_info->_icon;

    const QVector<MsgInfo>& msgList = pTextEdit->getMsgList();
    for(int i=0; i<msgList.size(); ++i)
    {
        QString type = msgList[i].msgFlag;
        ChatItemBase *pChatItem = new ChatItemBase(role);
        pChatItem->setUserName(userName);
        pChatItem->setUserIcon(QPixmap(userIcon));
        QWidget *pBubble = nullptr;
        if(type == "text")
        {
            pBubble = new TextBubble(role, msgList[i].content);
        }
        else if(type == "image")
        {
            pBubble = new PictureBubble(QPixmap(msgList[i].content) , role);
        }
        else if(type == "file")
        {

        }
        if(pBubble != nullptr)
        {
            pChatItem->setWidget(pBubble);
            ui->chat_data_list->appendChatItem(pChatItem);
        }
    }
}

void ChatPage::clearItems()
{
    ui->chat_data_list->removeAllItem();
}

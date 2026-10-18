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
#include "filebubble.h"
#include "filetransfermgr.h"
#include <QFileDialog>
#include <QFileInfo>
#include <QUuid>

ChatPage::ChatPage(QWidget* parent)
    : QWidget(parent),
      ui(new Ui::ChatPage)
{
    ui->setupUi(this);
    ui->send_btn->setAccent(QColor(0x0A, 0x84, 0xFF));

    // set the icon style
    ui->emo_lb->SetState("normal", "hover", "press", "normal", "hover", "press");
    ui->file_lb->SetState("normal", "hover", "press", "normal", "hover", "press");
    ui->emo_lb->SetIcons(ElaIconType::FaceSmile, ElaIconType::FaceSmile, 20);
    ui->file_lb->SetIcons(ElaIconType::Paperclip, ElaIconType::Paperclip, 20);

    // add the Enter-to-send-message signal
    connect(ui->chatEdit, &MessageTextEdit::send, this, &ChatPage::on_send_btn_clicked);
    connect(ui->file_lb, &ClickedLabel::clicked, this, &ChatPage::on_file_lb_clicked);
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
    _file_bubbles.clear();
    _file_msgs.clear();
    for (auto& msg : user_info->_chat_msgs)
    {
        AppendChatMsg(msg);
    }
}

void ChatPage::AppendChatMsg(std::shared_ptr<TextChatData> msg)
{
    auto self_info = UserMgr::GetInstance()->GetUserInfo();
    ChatRole role;
    if (msg->_from_uid == self_info->_uid)
    {
        role = ChatRole::Self;
        ChatItemBase* pChatItem = new ChatItemBase(role);

        pChatItem->setUserName(self_info->_name);
        pChatItem->setUserIcon(QPixmap(self_info->_icon));

        QWidget* pBubble = MakeBubble(role, msg);
        pChatItem->setWidget(pBubble);
        ui->chat_data_list->appendChatItem(pChatItem);
    }
    else
    {
        role = ChatRole::Other;
        ChatItemBase* pChatItem = new ChatItemBase(role);
        auto friend_info = UserMgr::GetInstance()->GetFriendById(msg->_from_uid);
        if (friend_info == nullptr)
        {
            delete pChatItem;
            return;
        }

        pChatItem->setUserName(friend_info->_name);
        pChatItem->setUserIcon(QPixmap(friend_info->_icon));

        QWidget* pBubble = MakeBubble(role, msg);
        pChatItem->setWidget(pBubble);
        ui->chat_data_list->appendChatItem(pChatItem);
    }
}

QWidget* ChatPage::MakeBubble(ChatRole role, std::shared_ptr<TextChatData> msg)
{
    if (msg->_msg_type != "file")
        return new TextBubble(role, msg->_msg_content);

    return AddFileBubble(role, msg);
}

FileBubble* ChatPage::AddFileBubble(ChatRole role, std::shared_ptr<TextChatData> msg)
{
    auto* bubble = new FileBubble(role, msg->_msg_content, msg->_file_size);
    if (!msg->_local_path.isEmpty())
    {
        bubble->setPath(msg->_local_path);
        bubble->setStatus(role == ChatRole::Self ? QStringLiteral("已发送 · 点击打开")
                                                 : QStringLiteral("已保存 · 点击打开"));
    }
    else if (role == ChatRole::Self)
        bubble->setStatus(QStringLiteral("已发送"));
    else
        bubble->setStatus(QStringLiteral("未接收"));

    _file_bubbles.insert(msg->_msg_id, bubble);
    _file_msgs.insert(msg->_msg_id, msg);
    return bubble;
}

void ChatPage::UpdateFileProgress(const QString& file_id, int percent)
{
    auto it = _file_bubbles.find(file_id);
    if (it != _file_bubbles.end())
        it.value()->setStatus(QStringLiteral("传输中 %1%").arg(percent));
}

void ChatPage::UpdateFileFinished(const QString& file_id, const QString& path)
{
    auto bubble = _file_bubbles.find(file_id);
    if (bubble == _file_bubbles.end())
        return;
    auto msg = _file_msgs.value(file_id);
    const bool is_self = msg != nullptr && msg->_from_uid == UserMgr::GetInstance()->GetUserInfo()->_uid;
    bubble.value()->setPath(path);
    bubble.value()->setStatus(is_self ? QStringLiteral("已发送 · 点击打开")
                                      : QStringLiteral("已保存 · 点击打开"));
    // the history entry is shared, so the path survives a reopen of the chat
    if (msg != nullptr)
        msg->_local_path = path;
}

void ChatPage::UpdateFileFailed(const QString& file_id, const QString& reason)
{
    auto it = _file_bubbles.find(file_id);
    if (it != _file_bubbles.end())
        it.value()->setFailed(reason);
}

void ChatPage::on_file_lb_clicked()
{
    if (_user_info == nullptr)
        return;
    const QStringList files = QFileDialog::getOpenFileNames(this, QStringLiteral("选择要发送的文件"));
    if (!files.isEmpty())
        ui->chatEdit->insertFileFromUrl(files);
}

void ChatPage::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QStyleOption opt;
    opt.initFrom(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

void ChatPage::on_send_btn_clicked()
{
    if (_user_info == nullptr)
    {
        qDebug() << "friend_info is empty";
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
    for (int i = 0; i < msgList.size(); ++i)
    {
        // skip if the message body exceeds the max length
        if (msgList[i].content.length() > 1024)
            continue;

        QString type = msgList[i].msgFlag;
        ChatItemBase* pChatItem = new ChatItemBase(role);

        pChatItem->setUserName(userName);
        pChatItem->setUserIcon(QPixmap(userIcon));

        QWidget* pBubble = nullptr;
        if (type == "text")
        {
            QUuid uuid = QUuid::createUuid();
            QString uuidString = uuid.toString();

            pBubble = new TextBubble(role, msgList[i].content);
            // send the accumulated messages and reset.
            if (txt_size + msgList[i].content.length() > 1024)
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

                emit TcpMgr::GetInstance() -> sig_send_data(ReqId::ID_TEXT_CHAT_MSG_REQ, jsonData);
            }

            // bind the bubble to the uid so delivery can be marked once the network replies
            //  _bubble_map[uuidString] = pBubble;
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
        else if (type == "image")
        {
            pBubble = new PictureBubble(QPixmap(msgList[i].content), role);
        }
        else if (type == "file")
        {
            const QString path = msgList[i].content;
            const QFileInfo info(path);
            QString send_error;
            const QString file_id =
                FileTransferMgr::GetInstance()->SendFile(_user_info->_uid, path, &send_error);

            auto file_msg = std::make_shared<TextChatData>(file_id, info.fileName(), user_info->_uid,
                                                           _user_info->_uid, "file");
            file_msg->_local_path = path;
            file_msg->_file_size = info.size();

            if (file_id.isEmpty())
            {
                // rejected before a byte went out; the manager already emitted the reason
                auto* bubble = new FileBubble(role, info.fileName(), info.size());
                bubble->setFailed(send_error.isEmpty() ? QStringLiteral("发送失败") : send_error);
                pChatItem->setWidget(bubble);
                ui->chat_data_list->appendChatItem(pChatItem);
                continue;
            }

            auto* bubble = AddFileBubble(role, file_msg);
            bubble->setPath(path);
            bubble->setStatus(QStringLiteral("传输中 0%"));
            pChatItem->setWidget(bubble);
            ui->chat_data_list->appendChatItem(pChatItem);
            emit sig_append_send_chat_msg(file_msg);
            continue;
        }
        // :) add the bubble to the chat page
        if (pBubble != nullptr)
        {
            pChatItem->setWidget(pBubble);
            ui->chat_data_list->appendChatItem(pChatItem);
        }
    }

    if (textArray.isEmpty())
        return;

    qDebug() << "textArray is " << textArray;
    // send to the server
    textObj["text_array"] = textArray;
    textObj["fromuid"] = user_info->_uid;
    textObj["touid"] = _user_info->_uid;
    QJsonDocument doc(textObj);
    QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
    // send and clear the previously accumulated text list
    txt_size = 0;
    textArray = QJsonArray();
    textObj = QJsonObject();
    // send the TCP request to the chat server
    emit TcpMgr::GetInstance() -> sig_send_data(ReqId::ID_TEXT_CHAT_MSG_REQ, jsonData);
}

void ChatPage::clearItems()
{
    ui->chat_data_list->removeAllItem();
}

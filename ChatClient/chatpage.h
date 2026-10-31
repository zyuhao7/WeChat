#ifndef CHATPAGE_H
#define CHATPAGE_H

#include <QWidget>
#include <QMap>
#include "global.h"
#include "userdata.h"

namespace Ui
{
class ChatPage;
}

class FileBubble;
class ChatItemBase;

class ChatPage : public QWidget
{
    Q_OBJECT

public:
    explicit ChatPage(QWidget* parent = nullptr);
    ~ChatPage();
    void SetUserInfo(std::shared_ptr<UserInfo> user_info);
    void AppendChatMsg(std::shared_ptr<TextChatData> msg);
    void PrependChatMsgs(const std::vector<std::shared_ptr<TextChatData>>& msgs);
    // a transfer whose chat is open learns its percentage and then its final path without
    // rebuilding the history
    void UpdateFileProgress(const QString& file_id, int percent);
    void UpdateFileFinished(const QString& file_id, const QString& path);
    void UpdateFileFailed(const QString& file_id, const QString& reason);

protected:
    void paintEvent(QPaintEvent* event);

private slots:
    void on_send_btn_clicked();
    void on_file_lb_clicked();

private:
    void clearItems();
    ChatItemBase* MakeChatItem(std::shared_ptr<TextChatData> msg);
    QWidget* MakeBubble(ChatRole role, std::shared_ptr<TextChatData> msg);
    FileBubble* AddFileBubble(ChatRole role, std::shared_ptr<TextChatData> msg);
    Ui::ChatPage* ui;
    std::shared_ptr<UserInfo> _user_info;
    bool _reconnecting = false;
    QMap<QString, QWidget*> _bubble_map;
    QHash<QString, FileBubble*> _file_bubbles;                // keyed by file id
    QHash<QString, std::shared_ptr<TextChatData>> _file_msgs; // so the path can stick
signals:
    void sig_append_send_chat_msg(std::shared_ptr<TextChatData> msg);
    void sig_load_more_history(); // the view reached the oldest loaded message
};

#endif // CHATPAGE_H

#ifndef CHATDIALOG_H
#define CHATDIALOG_H
#include "global.h"
#include "statewidget.h"
#include "userdata.h"
#include <QDialog>
#include <QLabel>
#include <QList>
#include <QListWidgetItem>
#include <memory>

namespace Ui {
class ChatDialog;
}

class   ChatDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ChatDialog(QWidget *parent = nullptr);
    ~ChatDialog();

protected:
    bool eventFilter(QObject * watch, QEvent * event) override;
    void handleGlobalMousePress(QMouseEvent* event);
    void CloseFindDlg();
     void UpdateChatMsg(std::vector<std::shared_ptr<TextChatData>> msgdata); // 更新聊天记录

private:
    void ShowSearch(bool bsearch = false);
    void AddLBGroup(StateWidget* lb);
    void addChatUserList();
    void loadMoreChatUser();
    void loadMoreConUser();
    void ClearLabelState(StateWidget *lb);
    void SetSelectChatItem(int uid = 0);
    void SetSelectChatPage(int uid = 0);

    Ui::ChatDialog *ui;
    ChatUIMode _mode;  // 控制 sidebar 作切换
    ChatUIMode _state; // 在同一模式也有不同状态
    bool _b_loading;
    QList<StateWidget*> _lb_list;
    QWidget* _last_widget;
    QMap<int, QListWidgetItem*> _chat_items_added;
    int _cur_chat_uid;

public slots:
    void slot_loading_chat_user();
    void slot_loading_contact_user();
    void slot_side_chat();
    void slot_side_contact();

    void slot_show_search(bool show);
    void slot_text_changed(const QString& str); // 搜索框内容发生变化
    void slot_focus_out(); // 搜索框失去焦点

    void slot_switch_apply_friend_page();
    void slot_apply_friend(std::shared_ptr<AddFriendApply>);
    void slot_add_auth_friend(std::shared_ptr<AuthInfo> auth_info);
    void slot_auth_rsp(std::shared_ptr<AuthRsp> auth_rsp);

    void slot_jump_chat_item(std::shared_ptr<SearchInfo> si);
    void slot_jump_chat_item_from_infopage(std::shared_ptr<UserInfo> user_info);
    void slot_friend_info_page(std::shared_ptr<UserInfo> user_info);

    void slot_item_clicked(QListWidgetItem* item);;
    void slot_text_chat_msg(std::shared_ptr<TextChatMsg> msg); // 处理收到的聊天消息
    void slot_append_send_chat_msg(std::shared_ptr<TextChatData> msg_data); // 追加发送的消息
};

#endif // CHATDIALOG_H

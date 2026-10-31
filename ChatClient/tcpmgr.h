#ifndef TCPMGR_H
#define TCPMGR_H
#include <QTcpSocket>
#include <QTimer>
#include <functional>
#include <QObject>
#include "singleton.h"
#include "global.h"
#include "userdata.h"

// Exponential backoff for an interrupted chat connection: 1s, 2s, 4s ... capped at 30s.
namespace TcpReconnect
{
constexpr int kBaseDelayMs = 1000;
constexpr int kMaxDelayMs = 30000;
constexpr int kMaxAttempts = 8;

inline int delayMs(int attempt)
{
    if (attempt <= 0)
        return 0;
    int delay = kBaseDelayMs;
    for (int i = 1; i < attempt && delay < kMaxDelayMs; ++i)
        delay *= 2;
    return delay > kMaxDelayMs ? kMaxDelayMs : delay;
}
} // namespace TcpReconnect

/******************************************************************************
 *
 * @file       tcpmgr.h
 * @brief      network message send/receive manager
 *
 * @author     Moyuhao
 * @date       2024/12/3
 * @history
 *****************************************************************************/

class TcpMgr : public QObject, public Singleton<TcpMgr>, public std::enable_shared_from_this<TcpMgr>
{
    Q_OBJECT
public:
    ~TcpMgr();
    void CloseConnection();

private:
    friend class Singleton<TcpMgr>;
    TcpMgr();
    void initHandlers();
    void HandleMsg(ReqId id, int len, QByteArray data);
    void ScheduleReconnect();
    void SendChatLogin();

    QTcpSocket _socket;
    QString _host;
    uint16_t _port;
    QByteArray _buffer;
    bool _b_recv_pending;
    quint16 _message_id;
    quint16 _message_len;
    QMap<ReqId, std::function<void(ReqId id, int len, QByteArray data)>> _handlers;

    ServerInfo _server_info;
    QTimer _reconnect_timer;
    int _reconnect_attempt;
    bool _logged_in;
    bool _reconnecting;
    bool _reconnect_suppressed;
    bool _manual_close;
    bool _kicked;
public slots:
    void slot_tcp_connect(ServerInfo);
    void slot_send_data(ReqId reqId, QByteArray data);

signals:
    void sig_con_success(bool bsuccess);
    void sig_send_data(ReqId reqId, QByteArray data);
    void sig_swich_chatdlg();
    void sig_load_apply_list(QJsonArray json_array);
    void sig_login_failed(int);
    void sig_user_search(std::shared_ptr<SearchInfo>);
    void sig_friend_apply(std::shared_ptr<AddFriendApply>);
    void sig_add_auth_friend(std::shared_ptr<AuthInfo>);
    void sig_auth_rsp(std::shared_ptr<AuthRsp>);
    void sig_text_chat_msg(std::shared_ptr<TextChatMsg> msg);
    void sig_chat_history(std::shared_ptr<ChatHistoryRsp> rsp);
    void sig_file_rsp(int error, QString fileid, int index);
    void sig_file_msg(QJsonObject obj);
    void sig_notify_offline();
    void sig_connection_closed();
    void sig_reconnecting(int attempt, int delayMs);
    void sig_reconnected();
    void sig_reconnect_failed();
};

#endif // TCPMGR_H

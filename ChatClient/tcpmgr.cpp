#include "tcpmgr.h"
#include "usermgr.h"
#include <QAbstractSocket>
#include <QJsonArray>
#include <QJsonDocument>

TcpMgr::~TcpMgr()
{
}

void TcpMgr::CloseConnection()
{
    _socket.abort();
}

TcpMgr::TcpMgr()
    : _host(""), _port(0), _b_recv_pending(false), _message_id(0), _message_len(0)
{
    QObject::connect(&_socket, &QTcpSocket::connected, [&]() {
        qDebug() << "Connected to server!";
        // send the message after the connection is established
        emit sig_con_success(true);
    });

    QObject::connect(&_socket, &QTcpSocket::readyRead, [&]() {
        // when data is readable, read all of it
        // read all data and append it to the buffer
        _buffer.append(_socket.readAll());

        forever
        {
            // parse the header first
            if (!_b_recv_pending)
            {
                // check whether the buffer has enough data to parse a header (msg id + length)
                if (_buffer.size() < static_cast<int>(sizeof(quint16) * 2))
                {
                    return; // not enough data; wait for more
                }

                // a stream reused across iterations keeps its read position after the buffer is
                // re-sliced, so each header is decoded from its own copy of the first four bytes
                QDataStream header(_buffer.left(sizeof(quint16) * 2));
                header.setVersion(QDataStream::Qt_5_0);
                header.setByteOrder(QDataStream::BigEndian);
                header >> _message_id >> _message_len;

                // remove the first four bytes from the buffer
                _buffer = _buffer.mid(sizeof(quint16) * 2);

                // output the read data
                qDebug() << "Message ID:" << _message_id << ", Length:" << _message_len;
            }

            // check whether the remaining buffer has the full body length; if not, return and keep waiting
            if (_buffer.size() < _message_len)
            {
                _b_recv_pending = true;
                return;
            }

            _b_recv_pending = false;
            // read the message body
            QByteArray messageBody = _buffer.mid(0, _message_len);
            qDebug() << "receive body msg is " << messageBody;

            _buffer = _buffer.mid(_message_len);
            HandleMsg(ReqId(_message_id), _message_len, messageBody);
        }
    });

    // versions after 5.15
    QObject::connect(&_socket, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::errorOccurred), [&](QAbstractSocket::SocketError socketError) {
        Q_UNUSED(socketError)
        qDebug() << "Error:" << _socket.errorString();
    });

    // QObject::connect(&_socket, &QTcpSocket::errorOccurred,
    //                  [&](QTcpSocket::SocketError socketError) {
    //                      qDebug() << "Error:" << _socket.errorString();
    //                      switch (socketError) {
    //                      case QTcpSocket::ConnectionRefusedError:
    //                          qDebug() << "Connection Refused!";
    //                          emit sig_con_success(false);
    //                          break;
    //                      case QTcpSocket::RemoteHostClosedError:
    //                          qDebug() << "Remote Host Closed Connection!";
    //                          break;
    //                      case QTcpSocket::HostNotFoundError:
    //                          qDebug() << "Host Not Found!";
    //                          emit sig_con_success(false);
    //                          break;
    //                      case QTcpSocket::SocketTimeoutError:
    //                          qDebug() << "Connection Timeout!";
    //                          emit sig_con_success(false);
    //                          break;
    //                      case QTcpSocket::NetworkError:
    //                          qDebug() << "Network Error!";
    //                          break;
    //                      default:
    //                          qDebug() << "Other Error!";
    //                          break;
    //                      }
    //                  });

    // handle the error (for Qt versions before 5.15)
    // QObject::connect(&_socket, static_cast<void (QTcpSocket::*)(QTcpSocket::SocketError)>(&QTcpSocket::error),
    //                     [&](QTcpSocket::SocketError socketError) {
    //        qDebug() << "Error:" << _socket.errorString() ;
    //        switch (socketError) {
    //            case QTcpSocket::ConnectionRefusedError:
    //                qDebug() << "Connection Refused!";
    //                emit sig_con_success(false);
    //                break;
    //            case QTcpSocket::RemoteHostClosedError:
    //                qDebug() << "Remote Host Closed Connection!";
    //                break;
    //            case QTcpSocket::HostNotFoundError:
    //                qDebug() << "Host Not Found!";
    //                emit sig_con_success(false);
    //                break;
    //            case QTcpSocket::SocketTimeoutError:
    //                qDebug() << "Connection Timeout!";
    //                emit sig_con_success(false);
    //                break;
    //            case QTcpSocket::NetworkError:
    //                qDebug() << "Network Error!";
    //                break;
    //            default:
    //                qDebug() << "Other Error!";
    //                break;
    //        }
    //  });

    // handle the disconnection
    QObject::connect(&_socket, &QTcpSocket::disconnected, [&]() {
        qDebug() << "Disconnected from server.";
        // drop any half-parsed frame so the next session starts clean
        _buffer.clear();
        _b_recv_pending = false;
        _message_id = 0;
        _message_len = 0;
        // emit a signal to notify the UI
        emit sig_connection_closed();
    });

    //  // connect the send signal used to send data
    QObject::connect(this, &TcpMgr::sig_send_data, this, &TcpMgr::slot_send_data);

    // register the message
    initHandlers();
}

void TcpMgr::initHandlers()
{
    _handlers.insert(ID_CHAT_LOGIN_RSP, [this](ReqId id, int len, QByteArray data) {
        Q_UNUSED(len);
        qDebug() << "handle id is " << id;
        // convert the QByteArray to a QJsonDocument
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);

        // check whether the conversion succeeded
        if (jsonDoc.isNull())
        {
            qDebug() << "Failed to create QJsonDocument.";
            return;
        }

        QJsonObject jsonObj = jsonDoc.object();
        qDebug() << "data jsonobj is " << jsonObj;

        if (!jsonObj.contains("error"))
        {
            int err = ErrorCodes::ERR_JSON;
            qDebug() << "Login Failed, err is Json Parse Err" << err;
            emit sig_login_failed(err);
            return;
        }

        int err = jsonObj["error"].toInt();
        if (err != ErrorCodes::SUCCESS)
        {
            qDebug() << "Login Failed, err is " << err;
            emit sig_login_failed(err);
            return;
        }

        auto uid = jsonObj["uid"].toInt();
        auto name = jsonObj["name"].toString();
        auto nick = jsonObj["nick"].toString();
        auto icon = jsonObj["icon"].toString();
        auto sex = jsonObj["sex"].toInt();
        auto desc = jsonObj["desc"].toString();
        auto user_info = std::make_shared<UserInfo>(uid, name, nick, icon, sex, "", desc);

        UserMgr::GetInstance()->SetUserInfo(user_info);
        UserMgr::GetInstance()->SetToken(jsonObj["token"].toString());

        if (jsonObj.contains("apply_list"))
        {
            UserMgr::GetInstance()->AppendApplyList(jsonObj["apply_list"].toArray());
        }

        // add the friend list
        if (jsonObj.contains("friend_list"))
        {
            UserMgr::GetInstance()->AppendFriendList(jsonObj["friend_list"].toArray());
        }

        emit sig_swich_chatdlg();
    });

    _handlers.insert(ID_SEARCH_USER_RSP, [this](ReqId id, int len, QByteArray data) {
        Q_UNUSED(len);
        qDebug() << "handle id is " << id;
        // convert the QByteArray to a QJsonDocument
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);

        // check whether the conversion succeeded
        if (jsonDoc.isNull())
        {
            qDebug() << "Failed to create QJsonDocument.";
            return;
        }

        QJsonObject jsonObj = jsonDoc.object();
        qDebug() << "data jsonobj is " << jsonObj;

        if (!jsonObj.contains("error"))
        {
            int err = ErrorCodes::ERR_JSON;
            qDebug() << "Search User Failed, err is Json Parse Err" << err;
            emit sig_user_search(nullptr);
            return;
        }

        int err = jsonObj["error"].toInt();
        if (err != ErrorCodes::SUCCESS)
        {
            qDebug() << "Search User Failed, err is " << err;
            emit sig_user_search(nullptr);
            return;
        }

        auto search_info = std::make_shared<SearchInfo>(jsonObj["uid"].toInt(), jsonObj["name"].toString(),
                                                        jsonObj["nick"].toString(), jsonObj["desc"].toString(),
                                                        jsonObj["sex"].toInt(), jsonObj["icon"].toString());

        emit sig_user_search(search_info);
    });
    // A -> B response returned to A's client
    _handlers.insert(ID_ADD_FRIEND_RSP, [this](ReqId id, int len, QByteArray data) {
        Q_UNUSED(len);
        qDebug() << "handle id is " << id;
        // convert the QByteArray to a QJsonDocument
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);

        // check whether the conversion succeeded
        if (jsonDoc.isNull())
        {
            qDebug() << "Failed to create QJsonDocument.";
            return;
        }

        QJsonObject jsonObj = jsonDoc.object();
        qDebug() << "data jsonobj is " << jsonObj;

        if (!jsonObj.contains("error"))
        {
            int err = ErrorCodes::ERR_JSON;
            qDebug() << "Add Friend RSP Failed, err is Json Parse Err" << err;
            return;
        }

        int err = jsonObj["error"].toInt();
        if (err != ErrorCodes::SUCCESS)
        {
            qDebug() << "Add Friend RSP Failed, err is " << err;
            return;
        }

        qDebug() << "add Friend RSP success!";
    });
    // A -> B  response returned to B's client
    _handlers.insert(ID_NOTIFY_ADD_FRIEND_REQ, [this](ReqId id, int len, QByteArray data) {
        Q_UNUSED(len);
        qDebug() << "handle id is " << id;
        // convert the QByteArray to a QJsonDocument
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);

        // check whether the conversion succeeded
        if (jsonDoc.isNull())
        {
            qDebug() << "Failed to create QJsonDocument.";
            return;
        }

        QJsonObject jsonObj = jsonDoc.object();
        qDebug() << "data jsonobj is " << jsonObj;

        if (!jsonObj.contains("error"))
        {
            int err = ErrorCodes::ERR_JSON;
            qDebug() << "notify add friend Failed, err is Json Parse Err" << err;
            emit sig_friend_apply(nullptr);
            return;
        }

        int err = jsonObj["error"].toInt();
        if (err != ErrorCodes::SUCCESS)
        {
            qDebug() << "notify add friend Failed, err is " << err;
            emit sig_friend_apply(nullptr);
            return;
        }

        int from_uid = jsonObj["applyuid"].toInt();
        auto name = jsonObj["name"].toString();
        auto nick = jsonObj["nick"].toString();
        auto desc = jsonObj["desc"].toString();
        auto icon = jsonObj["icon"].toString();
        auto sex = jsonObj["sex"].toInt();

        auto apply_info = std::make_shared<AddFriendApply>(from_uid, name, desc, icon, nick, sex);

        emit sig_friend_apply(apply_info);

        qDebug() << "notify add friend success!";
    });

    _handlers.insert(ID_NOTIFY_AUTH_FRIEND_REQ, [this](ReqId id, int len, QByteArray data) {
        Q_UNUSED(len);
        qDebug() << "handle id is " << id << " data is " << data;
        // convert the QByteArray to a QJsonDocument
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);

        // check whether the conversion succeeded
        if (jsonDoc.isNull())
        {
            qDebug() << "Failed to create QJsonDocument.";
            return;
        }

        QJsonObject jsonObj = jsonDoc.object();
        if (!jsonObj.contains("error"))
        {
            int err = ErrorCodes::ERR_JSON;
            qDebug() << "Auth Friend Failed, err is " << err;
            return;
        }

        int err = jsonObj["error"].toInt();
        if (err != ErrorCodes::SUCCESS)
        {
            qDebug() << "Auth Friend Failed, err is " << err;
            return;
        }

        int from_uid = jsonObj["fromuid"].toInt();
        QString name = jsonObj["name"].toString();
        QString nick = jsonObj["nick"].toString();
        QString icon = jsonObj["icon"].toString();
        int sex = jsonObj["sex"].toInt();

        auto auth_info = std::make_shared<AuthInfo>(from_uid, name,
                                                    nick, icon, sex);

        emit sig_add_auth_friend(auth_info);
    });

    _handlers.insert(ID_AUTH_FRIEND_RSP, [this](ReqId id, int len, QByteArray data) {
        Q_UNUSED(len);
        qDebug() << "handle id is " << id << " data is " << data;
        // convert the QByteArray to a QJsonDocument
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);

        // check whether the conversion succeeded
        if (jsonDoc.isNull())
        {
            qDebug() << "Failed to create QJsonDocument.";
            return;
        }

        QJsonObject jsonObj = jsonDoc.object();

        if (!jsonObj.contains("error"))
        {
            int err = ErrorCodes::ERR_JSON;
            qDebug() << "Auth Friend Failed, err is Json Parse Err" << err;
            return;
        }

        int err = jsonObj["error"].toInt();
        if (err != ErrorCodes::SUCCESS)
        {
            qDebug() << "Auth Friend Failed, err is " << err;
            return;
        }

        auto name = jsonObj["name"].toString();
        auto nick = jsonObj["nick"].toString();
        auto icon = jsonObj["icon"].toString();
        auto sex = jsonObj["sex"].toInt();
        auto uid = jsonObj["uid"].toInt();
        auto rsp = std::make_shared<AuthRsp>(uid, name, nick, icon, sex);
        emit sig_auth_rsp(rsp);

        qDebug() << "Auth Friend Success ";
    });

    _handlers.insert(ID_TEXT_CHAT_MSG_RSP, [](ReqId id, int len, QByteArray data) {
        Q_UNUSED(len);
        qDebug() << "handle id is " << id << " data is " << data;
        // convert the QByteArray to a QJsonDocument
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);

        // check whether the conversion succeeded
        if (jsonDoc.isNull())
        {
            qDebug() << "Failed to create QJsonDocument.";
            return;
        }

        QJsonObject jsonObj = jsonDoc.object();

        if (!jsonObj.contains("error"))
        {
            int err = ErrorCodes::ERR_JSON;
            qDebug() << "Chat Msg Rsp Failed, err is Json Parse Err" << err;
            return;
        }

        int err = jsonObj["error"].toInt();
        if (err != ErrorCodes::SUCCESS)
        {
            qDebug() << "Chat Msg Rsp Failed, err is " << err;
            return;
        }

        qDebug() << "Receive Text Chat Rsp Success ";
    });

    _handlers.insert(ID_NOTIFY_TEXT_CHAT_MSG_REQ, [this](ReqId id, int len, QByteArray data) {
        Q_UNUSED(len);
        qDebug() << "handle id is " << id << " data is " << data;
        // convert the QByteArray to a QJsonDocument
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);

        // check whether the conversion succeeded
        if (jsonDoc.isNull())
        {
            qDebug() << "Failed to create QJsonDocument.";
            return;
        }

        QJsonObject jsonObj = jsonDoc.object();

        if (!jsonObj.contains("error"))
        {
            int err = ErrorCodes::ERR_JSON;
            qDebug() << "Notify Chat Msg Failed, err is Json Parse Err" << err;
            return;
        }

        int err = jsonObj["error"].toInt();
        if (err != ErrorCodes::SUCCESS)
        {
            qDebug() << "Notify Chat Msg Failed, err is " << err;
            return;
        }

        qDebug() << "Receive Text Chat Notify Success ";
        auto msg_ptr = std::make_shared<TextChatMsg>(jsonObj["fromuid"].toInt(),
                                                     jsonObj["touid"].toInt(), jsonObj["text_array"].toArray());
        emit sig_text_chat_msg(msg_ptr);
    });

    _handlers.insert(ID_GET_CHAT_HISTORY_RSP, [this](ReqId id, int len, QByteArray data) {
        Q_UNUSED(id);
        Q_UNUSED(len);
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
        if (!jsonDoc.isObject())
        {
            qDebug() << "Chat history rsp is not a JSON object";
            return;
        }

        QJsonObject jsonObj = jsonDoc.object();
        const int err = jsonObj.value("error").toInt(ErrorCodes::ERR_JSON);
        if (err != ErrorCodes::SUCCESS)
        {
            qDebug() << "Chat history rsp failed, err is " << err;
            return;
        }

        auto rsp = std::make_shared<ChatHistoryRsp>();
        rsp->_peer_uid = jsonObj.value("touid").toInt();
        const QJsonArray arrays = jsonObj.value("text_array").toArray();
        for (const QJsonValue& value : arrays)
        {
            const QJsonObject item = value.toObject();
            auto msg = std::make_shared<TextChatData>(item.value("msgid").toString(),
                                                      item.value("content").toString(),
                                                      item.value("fromuid").toInt(),
                                                      item.value("touid").toInt());
            msg->_seq = static_cast<qint64>(item.value("seq").toDouble());
            rsp->_chat_msgs.push_back(msg);
        }
        emit sig_chat_history(rsp);
    });

    _handlers.insert(ID_HEART_BEAT_RSP, [](ReqId id, int len, QByteArray data) {
        Q_UNUSED(len);
        qDebug() << "handler id is " << id << " data is : " << data;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);

        if (jsonDoc.isNull())
        {
            qDebug() << "Failed to create QJsonDocument.";
            return;
        }

        QJsonObject jsonObj = jsonDoc.object();
        if (!jsonObj.contains("error"))
        {
            int err = ErrorCodes::ERR_JSON;
            qDebug() << "Heart Beat Msg Failed, err is Json Parse Err" << err;
            return;
        }

        qDebug() << " Receive Heart Beat Msg Success";
    });

    // the server replies to every file chunk, successful or not, so each reply retires exactly
    // one index from the sender's window
    _handlers.insert(ID_FILE_MSG_RSP, [this](ReqId id, int len, QByteArray data) {
        Q_UNUSED(id);
        Q_UNUSED(len);
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
        if (jsonDoc.isNull() || !jsonDoc.isObject())
        {
            qDebug() << "File Msg Rsp Failed, err is Json Parse Err";
            return;
        }

        QJsonObject jsonObj = jsonDoc.object();
        int err = jsonObj.contains("error") ? jsonObj["error"].toInt() : ErrorCodes::ERR_JSON;
        emit sig_file_rsp(err, jsonObj["fileid"].toString(), jsonObj["index"].toInt(-1));
    });

    _handlers.insert(ID_NOTIFY_FILE_MSG_REQ, [this](ReqId id, int len, QByteArray data) {
        Q_UNUSED(id);
        Q_UNUSED(len);
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
        if (jsonDoc.isNull() || !jsonDoc.isObject())
        {
            qDebug() << "Notify File Msg Failed, err is Json Parse Err";
            return;
        }

        QJsonObject jsonObj = jsonDoc.object();
        int err = jsonObj.contains("error") ? jsonObj["error"].toInt() : ErrorCodes::ERR_JSON;
        if (err != ErrorCodes::SUCCESS)
        {
            qDebug() << "Notify File Msg Failed, err is " << err;
            return;
        }
        emit sig_file_msg(jsonObj);
    });
}

void TcpMgr::HandleMsg(ReqId id, int len, QByteArray data)
{
    auto find_iter = _handlers.find(id);
    if (find_iter == _handlers.end())
    {
        qDebug() << "not found id [" << id << "] to handle";
        return;
    }

    find_iter.value()(id, len, data);
}

void TcpMgr::slot_tcp_connect(ServerInfo si)
{
    qDebug() << "receive tcp connect signal";
    // try to connect to the server
    qDebug() << "Connecting to server...";
    _host = si.Host;
    _port = static_cast<uint16_t>(si.Port.toUInt());
    // a stale socket (still closing after a heartbeat timeout, or mid
    // lookup for a previous host) turns connectToHost into a no-op.
    if (_socket.state() != QAbstractSocket::UnconnectedState)
    {
        _socket.abort();
    }
    _buffer.clear();
    _b_recv_pending = false;
    _message_id = 0;
    _message_len = 0;
    _socket.connectToHost(si.Host, _port);
}

void TcpMgr::slot_send_data(ReqId reqId, QByteArray dataBytes)
{
    uint16_t id = reqId;

    // compute the length (using network byte order)
    quint16 len = static_cast<quint16>(dataBytes.length());

    // create a QByteArray to hold all data to be sent
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);

    // set the data stream to use network byte order
    out.setByteOrder(QDataStream::BigEndian);

    // write the id and length
    out << id << len;

    // add string data
    block.append(dataBytes);

    // send data
    _socket.write(block);
    qDebug() << "tcp mgr send byte data is " << block;
}

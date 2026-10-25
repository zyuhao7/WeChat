#include "LogicSystem.h"
#include "MysqlMgr.h"
#include "const.h"
#include "RedisMgr.h"
#include "ConfigMgr.h"
#include "UserMgr.h"
#include "CServer.h"
#include "ChatGrpcClient.h"
#include "JsonUtil.h"
#include "UserInfoCache.h"
using namespace std;

LogicSystem::LogicSystem()
    : _b_stop(false),
      _p_server(nullptr)
{
    RegisterCallBacks();
    _worker_thread = std::thread(&LogicSystem::DealMsg, this);
}

LogicSystem::~LogicSystem()
{
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _b_stop = true;
    }
    _consume.notify_one();
    _worker_thread.join();
}

void LogicSystem::PostMsgToQue(std::shared_ptr<LogicNode> msg)
{
    {
        std::lock_guard<std::mutex> lock(_mutex);
        if (_msg_que.size() >= MAX_RECVQUE)
        {
            std::cout << "logic queue full, drop msg id " << msg->_recvnode->_msg_id << std::endl;
            return;
        }
        _msg_que.push(msg);
    }
    _consume.notify_one();
}

void LogicSystem::SetServer(std::shared_ptr<CServer> pserver)
{
    _p_server = pserver;
}

void LogicSystem::DealMsg()
{
    for (;;)
    {
        std::shared_ptr<LogicNode> msg_node;
        {
            std::unique_lock<std::mutex> unique_lk(_mutex);
            _consume.wait(unique_lk, [this]() { return _b_stop || !_msg_que.empty(); });
            // on stop, drain what is already queued before exiting
            if (_msg_que.empty())
                return;
            msg_node = _msg_que.front();
            _msg_que.pop();
        }
        // handlers block on Redis/MySQL/gRPC, so run them without holding the queue lock
        Dispatch(msg_node);
    }
}

void LogicSystem::Dispatch(const std::shared_ptr<LogicNode>& msg_node)
{
    auto msg_id = msg_node->_recvnode->_msg_id;
    cout << "recv_msg id is " << msg_id << endl;
    auto call_back_iter = _fun_callbacks.find(msg_id);
    if (call_back_iter == _fun_callbacks.end())
    {
        std::cout << "msg id [" << msg_id << "] handler not found !" << std::endl;
        return;
    }
    // this is the only logic thread: one bad message must not take it down
    try
    {
        call_back_iter->second(msg_node->_session, msg_id,
                               std::string(msg_node->_recvnode->_data, msg_node->_recvnode->_cur_len));
    }
    catch (const std::exception& e)
    {
        std::cout << "handler for msg id [" << msg_id << "] threw: " << e.what() << std::endl;
    }
}

void LogicSystem::RegisterCallBacks()
{
    _fun_callbacks[MSG_CHAT_LOGIN] = std::bind(&LogicSystem::LoginHandler, this,
                                               placeholders::_1, placeholders::_2, placeholders::_3);
    _fun_callbacks[ID_SEARCH_USER_REQ] = std::bind(&LogicSystem::SearchInfo, this,
                                                   placeholders::_1, placeholders::_2, placeholders::_3);
    _fun_callbacks[ID_ADD_FRIEND_REQ] = std::bind(&LogicSystem::AddFriendApply, this,
                                                  placeholders::_1, placeholders::_2, placeholders::_3);
    _fun_callbacks[ID_AUTH_FRIEND_REQ] = std::bind(&LogicSystem::AuthFriendApply, this,
                                                   placeholders::_1, placeholders::_2, placeholders::_3);
    _fun_callbacks[ID_TEXT_CHAT_MSG_REQ] = std::bind(&LogicSystem::DealChatTextMsg, this,
                                                     placeholders::_1, placeholders::_2, placeholders::_3);
    _fun_callbacks[ID_FILE_MSG_REQ] = std::bind(&LogicSystem::DealFileMsg, this,
                                                placeholders::_1, placeholders::_2, placeholders::_3);
    _fun_callbacks[ID_GET_CHAT_HISTORY_REQ] = std::bind(&LogicSystem::DealGetChatHistory, this,
                                                        placeholders::_1, placeholders::_2, placeholders::_3);
    _fun_callbacks[ID_HEART_BEAT_REQ] = std::bind(&LogicSystem::HeartBeatHandler, this,
                                                  placeholders::_1, placeholders::_2, placeholders::_3);
}

void LogicSystem::LoginHandler(shared_ptr<CSession> session, const short& msg_id, const string& msg_data)
{
    Json::Value rtvalue;
    rtvalue["error"] = ErrorCodes::Success;
    Defer defer([this, &rtvalue, session]() {
        std::string return_str = rtvalue.toStyledString();
        session->Send(return_str, MSG_CHAT_LOGIN_RSP);
    });

    Json::Value root;
    if (!ParseJsonObject(msg_data, root))
    {
        rtvalue["error"] = ErrorCodes::Error_Json;
        return;
    }

    int uid = 0;
    std::string token;
    if (!GetIntField(root, "uid", uid) || !IsValidUid(uid))
    {
        rtvalue["error"] = ErrorCodes::UidInvalid;
        return;
    }
    if (!GetStringField(root, "token", token) || token.empty())
    {
        rtvalue["error"] = ErrorCodes::TokenInvalid;
        return;
    }
    std::cout << "user login uid is  " << uid << endl;

    // a connection carries one identity; switching user needs a new connection
    int bound_uid = session->GetUserId();
    if (bound_uid != 0 && bound_uid != uid)
    {
        rtvalue["error"] = ErrorCodes::UidInvalid;
        return;
    }

    // get the user token from redis to verify it
    std::string uid_str = std::to_string(uid);
    std::string token_key = USERTOKENPREFIX + uid_str;
    std::string token_value = "";
    bool success = RedisMgr::GetInstance()->Get(token_key, token_value);
    if (!success)
    {
        rtvalue["error"] = ErrorCodes::UidInvalid;
        return;
    }

    if (token_value != token)
    {
        rtvalue["error"] = ErrorCodes::TokenInvalid;
        return;
    }

    auto user_info = std::make_shared<UserInfo>();
    if (!LoadUserInfoByUid(uid, user_info))
    {
        rtvalue["error"] = ErrorCodes::UidInvalid;
        return;
    }

    auto server_name = ConfigMgr::Inst().GetValue("SelfServer", "Name");
    {
        // take a distributed lock here so this thread has exclusive login
        auto lock_key = LOCK_PREFIX + uid_str;
        auto identifier = RedisMgr::GetInstance()->acquireLock(lock_key, LOCK_TIME_OUT, ACQUIRE_TIME_OUT);
        // without the lock a concurrent login could interleave its kick and session writes with ours
        if (identifier.empty())
        {
            std::cout << "login uid " << uid << " failed to acquire lock" << std::endl;
            rtvalue["error"] = ErrorCodes::RPCFailed;
            return;
        }

        // unlock via defer
        Defer defer2([this, identifier, lock_key]() {
            RedisMgr::GetInstance()->releaseLock(lock_key, identifier);
        });

        // a repeated login on the same connection is not a new login: nothing to kick or count
        bool fresh_login = bound_uid != uid;
        if (fresh_login)
        {
            KickPreviousLogin(uid, session);
        }

        // bind the session to the user uid
        session->SetUserId(uid);

        // set the user's login server name
        std::string ipkey = USERIPPREFIX + uid_str;
        RedisMgr::GetInstance()->Set(ipkey, server_name);

        // manage uid to session binding to ease later kick operations
        UserMgr::GetInstance()->SetUserSession(uid, session);
        std::string uid_session_key = USER_SESSION_PREFIX + uid_str;
        RedisMgr::GetInstance()->Set(uid_session_key, session->GetSessionId());

        // reflect this login immediately so StatusServer's load balancer
        // can spread the next client across nodes without waiting for the
        // periodic reconcile timer
        if (fresh_login)
        {
            RedisMgr::GetInstance()->IncreaseCount(server_name);
        }
    }

    FillPublicUserInfo(*user_info, rtvalue);

    // get the apply list from the database
    std::vector<std::shared_ptr<ApplyInfo>> apply_list;
    auto b_apply = GetFriendApplyInfo(uid, apply_list);
    if (b_apply)
    {
        for (auto& apply : apply_list)
        {
            Json::Value obj;
            obj["name"] = apply->_name;
            obj["uid"] = apply->_uid;
            obj["icon"] = apply->_icon;
            obj["nick"] = apply->_nick;
            obj["sex"] = apply->_sex;
            obj["desc"] = apply->_desc;
            obj["status"] = apply->_status;
            rtvalue["apply_list"].append(obj);
        }
    }

    // get the friend list
    std::vector<std::shared_ptr<UserInfo>> friend_list;
    GetFriendList(uid, friend_list);
    for (auto& friend_ele : friend_list)
    {
        Json::Value obj;
        obj["name"] = friend_ele->name;
        obj["uid"] = friend_ele->uid;
        obj["icon"] = friend_ele->icon;
        obj["nick"] = friend_ele->nick;
        obj["sex"] = friend_ele->sex;
        obj["desc"] = friend_ele->desc;
        obj["back"] = friend_ele->back;
        rtvalue["friend_list"].append(obj);
    }
}

// Caller holds the uid's distributed lock.
void LogicSystem::KickPreviousLogin(int uid, const std::shared_ptr<CSession>& session)
{
    std::string uid_ip_value = "";
    bool b_ip = RedisMgr::GetInstance()->Get(USERIPPREFIX + std::to_string(uid), uid_ip_value);
    if (!b_ip)
        return;

    auto self_name = ConfigMgr::Inst()["SelfServer"]["Name"];
    // if the previous and current login servers are the same, kick on this server directly
    if (uid_ip_value == self_name)
    {
        auto old_session = UserMgr::GetInstance()->GetSession(uid);
        // the current connection may already be the mapped one; never kick ourselves
        if (old_session && old_session != session)
        {
            old_session->NotifyOffline(uid);
            _p_server->ClearSession(old_session->GetSessionId());
        }
        return;
    }

    // the previous login lives on another node; the redis value is that
    // node's name, which is also how ChatGrpcClient keys its pools
    KickUserReq kick_req;
    kick_req.set_uid(uid);
    auto kick_rsp = ChatGrpcClient::GetInstance()->NotifyKickUser(uid_ip_value, kick_req);
    if (kick_rsp.error() != ErrorCodes::Success)
    {
        std::cout << "kick uid " << uid << " on server " << uid_ip_value
                  << " failed, error is " << kick_rsp.error() << std::endl;
    }
}

void LogicSystem::SearchInfo(std::shared_ptr<CSession> session, const short& msg_id, const std::string& msg_data)
{
    Json::Value rtValue;
    rtValue["error"] = ErrorCodes::Success;
    Defer defer([this, &rtValue, session]() {
        std::string return_str = rtValue.toStyledString();
        session->Send(return_str, ID_SEARCH_USER_RSP);
    });

    // search results include the email, so anonymous connections may not enumerate users
    if (!IsValidUid(session->GetUserId()))
    {
        rtValue["error"] = ErrorCodes::UidInvalid;
        return;
    }

    Json::Value root;
    if (!ParseJsonObject(msg_data, root))
    {
        rtValue["error"] = ErrorCodes::Error_Json;
        return;
    }

    // the client sends the raw search box text as "uid": digits mean a uid, anything else a name
    std::string search_key;
    if (!GetStringField(root, "uid", search_key) || search_key.empty())
    {
        rtValue["error"] = ErrorCodes::UidInvalid;
        return;
    }
    std::cout << " user SearchInfo id is " << search_key << std::endl;

    auto user_info = std::make_shared<UserInfo>();
    bool found = false;
    if (IsPureDigit(search_key))
    {
        int uid = 0;
        found = ParseNonNegativeInt(search_key, uid) && LoadUserInfoByUid(uid, user_info);
    }
    else
    {
        found = LoadUserInfoByName(search_key, user_info);
    }

    if (!found)
    {
        rtValue["error"] = ErrorCodes::UidInvalid;
        return;
    }
    FillPublicUserInfo(*user_info, rtValue);
}

void LogicSystem::AddFriendApply(std::shared_ptr<CSession> session, const short& msg_id, const std::string& msg_data)
{
    Json::Value rtValue;
    rtValue["error"] = ErrorCodes::Success;

    Defer defer([this, &rtValue, session]() {
        std::string return_str = rtValue.toStyledString();
        session->Send(return_str, ID_ADD_FRIEND_RSP);
    });

    Json::Value root;
    if (!ParseJsonObject(msg_data, root))
    {
        rtValue["error"] = ErrorCodes::Error_Json;
        return;
    }

    // the applicant is whoever owns this connection, never a uid taken from the request
    int uid = session->GetUserId();
    int touid = 0;
    if (!IsValidUid(uid) || !GetIntField(root, "touid", touid) || !IsValidUid(touid) || touid == uid)
    {
        rtValue["error"] = ErrorCodes::UidInvalid;
        return;
    }
    std::string applyname;
    std::string bakname;
    GetStringField(root, "applyname", applyname);
    GetStringField(root, "bakname", bakname);
    std::cout << "user uid is " << uid << " applyname is " << applyname << " bakname is " << bakname << " touid is " << touid << std::endl;

    auto to_info = std::make_shared<UserInfo>();
    if (!LoadUserInfoByUid(touid, to_info))
    {
        rtValue["error"] = ErrorCodes::UidInvalid;
        return;
    }

    // update the database first
    if (!MysqlMgr::GetInstance()->AddFriendApply(uid, touid))
    {
        rtValue["error"] = ErrorCodes::RPCFailed;
        return;
    }

    // query redis for the server ip of touid
    auto to_str = std::to_string(touid);
    auto to_ip_key = USERIPPREFIX + to_str;
    std::string to_ip_value = "";
    bool b_ip = RedisMgr::GetInstance()->Get(to_ip_key, to_ip_value);
    if (!b_ip)
        return;

    auto& cfg = ConfigMgr::Inst();
    auto self_name = cfg["SelfServer"]["Name"];

    auto apply_info = std::make_shared<UserInfo>();
    bool b_info = LoadUserInfoByUid(uid, apply_info);

    // notify the peer directly about the apply message
    if (to_ip_value == self_name)
    {
        auto to_session = UserMgr::GetInstance()->GetSession(touid);
        if (to_session)
        {
            // notify the peer directly if in memory.
            Json::Value notify;
            notify["error"] = ErrorCodes::Success;
            notify["applyuid"] = uid;
            notify["name"] = applyname;
            notify["desc"] = "";
            if (b_info)
            {
                notify["icon"] = apply_info->icon;
                notify["sex"] = apply_info->sex;
                notify["nick"] = apply_info->nick;
            }
            std::string return_str = notify.toStyledString();
            to_session->Send(return_str, ID_NOTIFY_ADD_FRIEND_REQ);
        }
        return;
    }

    AddFriendReq add_req;
    add_req.set_applyuid(uid);
    add_req.set_touid(touid);
    add_req.set_name(applyname);
    add_req.set_desc("");
    if (b_info)
    {
        add_req.set_icon(apply_info->icon);
        add_req.set_sex(apply_info->sex);
        add_req.set_nick(apply_info->nick);
    }
    // send the notification
    ChatGrpcClient::GetInstance()->NotifyAddFriend(to_ip_value, add_req);
}

void LogicSystem::AuthFriendApply(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data)
{
    Json::Value rtvalue;
    rtvalue["error"] = ErrorCodes::Success;

    Defer defer([this, &rtvalue, session]() {
        std::string return_str = rtvalue.toStyledString();
        session->Send(return_str, ID_AUTH_FRIEND_RSP);
    });

    Json::Value root;
    if (!ParseJsonObject(msg_data, root))
    {
        rtvalue["error"] = ErrorCodes::Error_Json;
        return;
    }

    // the approver is whoever owns this connection, never a uid taken from the request
    int uid = session->GetUserId();
    int touid = 0;
    if (!IsValidUid(uid) || !GetIntField(root, "touid", touid) || !IsValidUid(touid) || touid == uid)
    {
        rtvalue["error"] = ErrorCodes::UidInvalid;
        return;
    }
    std::string back_name;
    GetStringField(root, "back", back_name);
    std::cout << "from " << uid << " auth friend to " << touid << std::endl;

    auto user_info = std::make_shared<UserInfo>();
    if (!LoadUserInfoByUid(touid, user_info))
    {
        rtvalue["error"] = ErrorCodes::UidInvalid;
        return;
    }

    // only a pending apply from touid can be approved; this also gates the friend insert
    if (!MysqlMgr::GetInstance()->AuthFriendApply(uid, touid))
    {
        rtvalue["error"] = ErrorCodes::UidInvalid;
        return;
    }

    if (!MysqlMgr::GetInstance()->AddFriend(uid, touid, back_name))
    {
        rtvalue["error"] = ErrorCodes::RPCFailed;
        return;
    }

    rtvalue["name"] = user_info->name;
    rtvalue["nick"] = user_info->nick;
    rtvalue["icon"] = user_info->icon;
    rtvalue["sex"] = user_info->sex;
    rtvalue["uid"] = touid;

    // query redis for the server ip of touid
    auto to_str = std::to_string(touid);
    auto to_ip_key = USERIPPREFIX + to_str;
    std::string to_ip_value = "";
    bool b_ip = RedisMgr::GetInstance()->Get(to_ip_key, to_ip_value);
    if (!b_ip)
    {
        return;
    }

    auto& cfg = ConfigMgr::Inst();
    auto self_name = cfg["SelfServer"]["Name"];
    // notify the peer directly about the auth-passed message
    if (to_ip_value == self_name)
    {
        auto to_session = UserMgr::GetInstance()->GetSession(touid);
        if (to_session)
        {
            // if in memory, notify the peer directly
            Json::Value notify;
            notify["error"] = ErrorCodes::Success;
            notify["fromuid"] = uid;
            notify["touid"] = touid;
            auto self_info = std::make_shared<UserInfo>();
            if (LoadUserInfoByUid(uid, self_info))
            {
                notify["name"] = self_info->name;
                notify["nick"] = self_info->nick;
                notify["icon"] = self_info->icon;
                notify["sex"] = self_info->sex;
            }
            else
            {
                notify["error"] = ErrorCodes::UidInvalid;
            }

            std::string return_str = notify.toStyledString();
            to_session->Send(return_str, ID_NOTIFY_AUTH_FRIEND_REQ);
        }

        return;
    }

    AuthFriendReq auth_req;
    auth_req.set_fromuid(uid);
    auth_req.set_touid(touid);

    // send the notification
    ChatGrpcClient::GetInstance()->NotifyAuthFriend(to_ip_value, auth_req);
}

void LogicSystem::DealChatTextMsg(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data)
{
    Json::Value rtvalue;
    rtvalue["error"] = ErrorCodes::Success;

    Defer defer([this, &rtvalue, session]() {
        std::string return_str = rtvalue.toStyledString();
        session->Send(return_str, ID_TEXT_CHAT_MSG_RSP);
    });

    Json::Value root;
    if (!ParseJsonObject(msg_data, root))
    {
        rtvalue["error"] = ErrorCodes::Error_Json;
        return;
    }

    // the sender is whoever owns this connection, never a uid taken from the request
    int uid = session->GetUserId();
    int touid = 0;
    if (!IsValidUid(uid) || !GetIntField(root, "touid", touid) || !IsValidUid(touid))
    {
        rtvalue["error"] = ErrorCodes::UidInvalid;
        return;
    }

    const Json::Value& arrays = root["text_array"];
    if (!IsValidTextArray(arrays))
    {
        rtvalue["error"] = ErrorCodes::Error_Json;
        return;
    }

    rtvalue["text_array"] = arrays;
    rtvalue["fromuid"] = uid;
    rtvalue["touid"] = touid;

    // store before forwarding: a peer that is offline has no session to notify, and this
    // row is what its client reads back when the conversation is opened again
    std::vector<std::pair<std::string, std::string>> rows;
    rows.reserve(arrays.size());
    for (const auto& item : arrays)
    {
        rows.emplace_back(item["msgid"].asString(), item["content"].asString());
    }
    if (!MysqlMgr::GetInstance()->AddChatMessages(uid, touid, rows))
    {
        // delivery still works while the store does; a failed write only loses history
        std::cerr << "persist chat messages from " << uid << " to " << touid << " failed" << std::endl;
    }

    // query redis for the server ip of touid
    auto to_str = std::to_string(touid);
    auto to_ip_key = USERIPPREFIX + to_str;
    std::string to_ip_value = "";
    bool b_ip = RedisMgr::GetInstance()->Get(to_ip_key, to_ip_value);
    if (!b_ip)
    {
        return;
    }

    auto& cfg = ConfigMgr::Inst();
    auto self_name = cfg["SelfServer"]["Name"];
    // notify the peer directly about the auth-passed message
    if (to_ip_value == self_name)
    {
        auto to_session = UserMgr::GetInstance()->GetSession(touid);
        if (to_session)
        {
            // if in memory, notify the peer directly
            std::string return_str = rtvalue.toStyledString();
            to_session->Send(return_str, ID_NOTIFY_TEXT_CHAT_MSG_REQ);
        }

        return;
    }

    TextChatMsgReq text_msg_req;
    text_msg_req.set_fromuid(uid);
    text_msg_req.set_touid(touid);
    for (const auto& txt_obj : arrays)
    {
        auto* text_msg = text_msg_req.add_textmsgs();
        text_msg->set_msgid(txt_obj["msgid"].asString());
        text_msg->set_msgcontent(txt_obj["content"].asString());
    }

    // send the notification
    ChatGrpcClient::GetInstance()->NotifyTextChatMsg(to_ip_value, text_msg_req, rtvalue);
}

// Returns stored messages between the caller and one peer, oldest first. The query always
// includes the caller's own uid, so the peer field cannot be used to read someone else's
// conversation.
void LogicSystem::DealGetChatHistory(std::shared_ptr<CSession> session, const short& msg_id,
                                     const string& msg_data)
{
    Json::Value rtvalue;
    rtvalue["error"] = ErrorCodes::Success;

    Defer defer([&rtvalue, session]() {
        session->Send(rtvalue.toStyledString(), ID_GET_CHAT_HISTORY_RSP);
    });

    Json::Value root;
    if (!ParseJsonObject(msg_data, root))
    {
        rtvalue["error"] = ErrorCodes::Error_Json;
        return;
    }

    const int uid = session->GetUserId();
    if (!IsValidUid(uid))
    {
        rtvalue["error"] = ErrorCodes::UidInvalid;
        return;
    }

    ChatHistoryQuery query;
    if (!ParseChatHistoryQuery(root, uid, query))
    {
        rtvalue["error"] = ErrorCodes::Error_Json;
        return;
    }

    // echoed even on failure so the client can clear the matching in-flight flag
    rtvalue["touid"] = query.peer_uid;

    std::vector<std::shared_ptr<ChatMessageInfo>> messages;
    bool has_more = false;
    if (!MysqlMgr::GetInstance()->GetChatHistory(uid, query.peer_uid, query.last_id, query.limit,
                                                 messages, has_more))
    {
        rtvalue["error"] = ErrorCodes::DBFailed;
        return;
    }

    Json::Value array(Json::arrayValue);
    // the wire frame length is a signed short; stop before the JSON envelope can overflow it
    constexpr size_t MAX_BODY_BYTES = 24 * 1024;
    size_t body_size = rtvalue.toStyledString().size();
    // walk newest first so a trimmed page keeps the recent messages; the older ones are still
    // reachable through the next upward page
    std::vector<Json::Value> kept;
    for (auto it = messages.rbegin(); it != messages.rend(); ++it)
    {
        const auto& msg = *it;
        Json::Value item;
        item["msgid"] = msg->_msg_id;
        item["content"] = msg->_content;
        item["fromuid"] = msg->_from_uid;
        item["touid"] = msg->_to_uid;
        item["seq"] = static_cast<Json::Int64>(msg->_seq);

        const size_t item_size = item.toStyledString().size();
        if (body_size + item_size > MAX_BODY_BYTES)
        {
            has_more = true;
            break;
        }

        kept.push_back(std::move(item));
        body_size += item_size;
    }
    for (auto it = kept.rbegin(); it != kept.rend(); ++it)
        array.append(*it);
    rtvalue["text_array"] = array;
    rtvalue["has_more"] = has_more;
}

// One chunk per frame: the sender spaces them out, so a whole file never sits in the logic
// thread's queue. The relay is stateless — a chunk the peer misses is a failed transfer on
// the sender, whose per-index reply is what reports it.
void LogicSystem::DealFileMsg(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data)
{
    Json::Value rtvalue;
    rtvalue["error"] = ErrorCodes::Success;

    int index = -1;
    // echo the chunk identity back even on a rejected frame, so the sender can retire it
    Json::Value root;
    if (ParseJsonObject(msg_data, root))
    {
        GetIntField(root, "index", index);
        std::string fileid;
        GetStringField(root, "fileid", fileid);
        rtvalue["fileid"] = fileid;
    }

    Defer defer([this, &rtvalue, &index, session]() {
        rtvalue["index"] = index;
        session->Send(rtvalue.toStyledString(), ID_FILE_MSG_RSP);
    });

    if (root.isNull())
    {
        rtvalue["error"] = ErrorCodes::Error_Json;
        return;
    }

    // the sender is whoever owns this connection, never a uid taken from the request
    int uid = session->GetUserId();
    int touid = 0;
    if (!IsValidUid(uid) || !GetIntField(root, "touid", touid) || !IsValidUid(touid) || touid == uid)
    {
        rtvalue["error"] = ErrorCodes::UidInvalid;
        return;
    }

    FileChunk chunk;
    if (!ParseFileChunk(root, chunk))
    {
        rtvalue["error"] = ErrorCodes::FileInvalid;
        return;
    }

    auto to_str = std::to_string(touid);
    auto to_ip_key = USERIPPREFIX + to_str;
    std::string to_ip_value = "";
    if (!RedisMgr::GetInstance()->Get(to_ip_key, to_ip_value))
    {
        rtvalue["error"] = ErrorCodes::UidInvalid;
        return;
    }

    Json::Value notify;
    notify["error"] = ErrorCodes::Success;
    notify["fromuid"] = uid;
    notify["touid"] = touid;
    notify["fileid"] = chunk.file_id;
    notify["filename"] = chunk.file_name;
    notify["filesize"] = chunk.file_size;
    notify["index"] = chunk.index;
    notify["total"] = chunk.total;
    notify["data"] = chunk.data;

    auto& cfg = ConfigMgr::Inst();
    auto self_name = cfg["SelfServer"]["Name"];
    if (to_ip_value == self_name)
    {
        auto to_session = UserMgr::GetInstance()->GetSession(touid);
        if (!to_session)
        {
            // offline mid-transfer: say so now instead of after the whole file is streamed
            rtvalue["error"] = ErrorCodes::UidInvalid;
            return;
        }
        to_session->Send(notify.toStyledString(), ID_NOTIFY_FILE_MSG_REQ);
        return;
    }

    FileMsgReq file_req;
    file_req.set_fromuid(uid);
    file_req.set_touid(touid);
    file_req.set_fileid(chunk.file_id);
    file_req.set_filename(chunk.file_name);
    file_req.set_filesize(chunk.file_size);
    file_req.set_index(chunk.index);
    file_req.set_total(chunk.total);
    file_req.set_data(chunk.data);

    FileMsgRsp rsp = ChatGrpcClient::GetInstance()->NotifyFileMsg(to_ip_value, file_req);
    rtvalue["error"] = rsp.error();
}

void LogicSystem::HeartBeatHandler(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data)
{
    std::cout << "receive heartbeat msg, uid is " << session->GetUserId() << std::endl;
    Json::Value rtvalue;
    rtvalue["error"] = ErrorCodes::Success;
    session->Send(rtvalue.toStyledString(), ID_HEARTBEAT_RSP);
}

bool LogicSystem::GetFriendApplyInfo(int to_uid, std::vector<std::shared_ptr<ApplyInfo>>& list)
{
    // get the friend apply list from mysql
    return MysqlMgr::GetInstance()->GetApplyList(to_uid, list, 0, 10);
}

bool LogicSystem::GetFriendList(int self_id, std::vector<std::shared_ptr<UserInfo>>& user_list)
{
    // get the friend list from mysql
    return MysqlMgr::GetInstance()->GetFriendList(self_id, user_list);
}

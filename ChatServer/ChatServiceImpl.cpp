#include "ChatServiceImpl.h"
#include "UserMgr.h"
#include "CSession.h"
#include <json/json.h>
#include <json/value.h>
#include <json/reader.h>
#include "RedisMgr.h"
#include "MysqlMgr.h"
#include "UserInfoCache.h"

ChatServiceImpl::ChatServiceImpl()
{
}

Status ChatServiceImpl::NotifyAddFriend(ServerContext* context, const AddFriendReq* request, AddFriendRsp* reply)
{
    // check whether the user is on this server
    auto touid = request->touid();
    auto session = UserMgr::GetInstance()->GetSession(touid);

    Defer defer([request, reply]() {
        reply->set_error(ErrorCodes::Success);
        reply->set_applyuid(request->applyuid());
        reply->set_touid(request->touid());
    });

    // return immediately if the user is not in memory
    if (session == nullptr)
    {
        return Status::OK;
    }

    // if in memory, notify the peer directly
    Json::Value rtvalue;
    rtvalue["error"] = ErrorCodes::Success;
    rtvalue["applyuid"] = request->applyuid();
    rtvalue["name"] = request->name();
    rtvalue["desc"] = request->desc();
    rtvalue["icon"] = request->icon();
    rtvalue["sex"] = request->sex();
    rtvalue["nick"] = request->nick();

    std::string return_str = rtvalue.toStyledString();

    session->Send(return_str, ID_NOTIFY_ADD_FRIEND_REQ);
    return Status::OK;
}

Status ChatServiceImpl::NotifyAuthFriend(ServerContext* context, const AuthFriendReq* request,
                                         AuthFriendRsp* reply)
{
    // check whether the user is on this server
    auto touid = request->touid();
    auto fromuid = request->fromuid();
    auto session = UserMgr::GetInstance()->GetSession(touid);

    Defer defer([request, reply]() {
        reply->set_error(ErrorCodes::Success);
        reply->set_fromuid(request->fromuid());
        reply->set_touid(request->touid());
    });

    // return immediately if the user is not in memory
    if (session == nullptr)
    {
        return Status::OK;
    }

    // if in memory, notify the peer directly
    Json::Value rtvalue;
    rtvalue["error"] = ErrorCodes::Success;
    rtvalue["fromuid"] = request->fromuid();
    rtvalue["touid"] = request->touid();

    auto user_info = std::make_shared<UserInfo>();
    bool b_info = LoadUserInfoByUid(fromuid, user_info);
    if (b_info)
    {
        rtvalue["name"] = user_info->name;
        rtvalue["nick"] = user_info->nick;
        rtvalue["icon"] = user_info->icon;
        rtvalue["sex"] = user_info->sex;
    }
    else
    {
        rtvalue["error"] = ErrorCodes::UidInvalid;
    }

    std::string return_str = rtvalue.toStyledString();

    session->Send(return_str, ID_NOTIFY_AUTH_FRIEND_REQ);
    return Status::OK;
}

Status ChatServiceImpl::NotifyTextChatMsg(::grpc::ServerContext* context,
                                          const TextChatMsgReq* request, TextChatMsgRsp* reply)
{
    // check whether the user is on this server
    auto touid = request->touid();
    auto session = UserMgr::GetInstance()->GetSession(touid);
    reply->set_error(ErrorCodes::Success);

    // return immediately if the user is not in memory
    if (session == nullptr)
    {
        return Status::OK;
    }

    // if in memory, notify the peer directly
    Json::Value rtvalue;
    rtvalue["error"] = ErrorCodes::Success;
    rtvalue["fromuid"] = request->fromuid();
    rtvalue["touid"] = request->touid();

    // organize the chat data into an array
    Json::Value text_array;
    for (auto& msg : request->textmsgs())
    {
        Json::Value element;
        element["content"] = msg.msgcontent();
        element["msgid"] = msg.msgid();
        text_array.append(element);
    }
    rtvalue["text_array"] = text_array;

    std::string return_str = rtvalue.toStyledString();

    session->Send(return_str, ID_NOTIFY_TEXT_CHAT_MSG_REQ);
    return Status::OK;
}

Status ChatServiceImpl::NotifyKickUser(::grpc::ServerContext* context, const KickUserReq* req, KickUserRsp* reply)
{
    // check whether the user is on this server
    auto uid = req->uid();

    // an unwired service must report an error rather than clear a session through a null server
    if (_p_server == nullptr)
    {
        reply->set_error(ErrorCodes::RPCFailed);
        reply->set_uid(uid);
        return Status::OK;
    }

    auto session = UserMgr::GetInstance()->GetSession(uid);

    Defer defer([req, reply]() {
        reply->set_error(ErrorCodes::Success);
        reply->set_uid(req->uid());
    });
    // return if the user is not in memory
    if (session == nullptr)
        return Status::OK;

    session->NotifyOffline(uid);
    _p_server->ClearSession(session->GetSessionId());
    return Status::OK;
}

void ChatServiceImpl::RegisterServer(std::shared_ptr<CServer> pServer)
{
    _p_server = pServer;
}

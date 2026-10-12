#include "StatusServiceImpl.h"
#include "ConfigMgr.h"
#include "const.h"
#include "RedisMgr.h"
#include "ServerPicker.h"
#include <map>

// The token is only checked when the client logs in to the chat server right after
// GetChatServer, so it only needs to outlive that hand-off.
constexpr int USER_TOKEN_TTL_SECONDS = 600;

std::string generate_unique_string()
{
    // create the UUID object
    boost::uuids::uuid uuid = boost::uuids::random_generator()();

    // convert the UUID to a string
    std::string unique_string = to_string(uuid);

    return unique_string;
}

Status StatusServiceImpl::GetChatServer(ServerContext* context, const GetChatServerReq* request, GetChatServerRsp* reply)
{
    ChatServer server;
    if (!getChatServer(server))
    {
        reply->set_error(ErrorCodes::RPCFailed);
        return Status::OK;
    }

    auto token = generate_unique_string();
    if (!insertToken(request->uid(), token))
    {
        reply->set_error(ErrorCodes::RPCFailed);
        return Status::OK;
    }

    reply->set_host(server.host);
    reply->set_port(server.port);
    reply->set_token(token);
    reply->set_error(ErrorCodes::Success);
    return Status::OK;
}

StatusServiceImpl::StatusServiceImpl()
{
    auto& cfg = ConfigMgr::Inst();
    for (auto& word : ParseServerList(cfg["chatservers"]["Name"]))
    {
        if (cfg[word]["Name"].empty())
        {
            continue;
        }

        ChatServer server;
        server.port = cfg[word]["Port"];
        server.host = cfg[word]["Host"];
        server.name = cfg[word]["Name"];
        _servers.push_back(server);
    }
}

bool StatusServiceImpl::getChatServer(ChatServer& picked)
{
    // a chat server publishes its count on startup and removes it on shutdown,
    // so a missing count means the server is down
    std::vector<std::string> names;
    std::map<std::string, std::string> counts;
    for (const auto& server : _servers)
    {
        names.push_back(server.name);
        counts[server.name] = RedisMgr::GetInstance()->HGet(LOGIN_COUNT, server.name);
    }

    auto name = PickServer(names, counts);
    if (!name)
    {
        return false;
    }
    for (const auto& server : _servers)
    {
        if (server.name == *name)
        {
            picked = server;
            return true;
        }
    }
    return false;
}

Status StatusServiceImpl::Login(ServerContext* context, const LoginReq* request, LoginRsp* reply)
{
    auto uid = request->uid();
    auto token = request->token();

    std::string uid_str = std::to_string(uid);
    std::string token_key = USERTOKENPREFIX + uid_str;
    std::string token_value = "";
    bool success = RedisMgr::GetInstance()->Get(token_key, token_value);
    if (!success)
    {
        reply->set_error(ErrorCodes::UidInvalid);
        return Status::OK;
    }

    if (token_value != token)
    {
        reply->set_error(ErrorCodes::TokenInvalid);
        return Status::OK;
    }
    reply->set_error(ErrorCodes::Success);
    reply->set_uid(uid);
    reply->set_token(token);
    return Status::OK;
}

bool StatusServiceImpl::insertToken(int uid, const std::string& token)
{
    std::string uid_str = std::to_string(uid);
    std::string token_key = USERTOKENPREFIX + uid_str;
    return RedisMgr::GetInstance()->SetEx(token_key, token, USER_TOKEN_TTL_SECONDS);
}

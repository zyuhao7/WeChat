#include "LogicSystem.h"

#include <json/json.h>

#include <iostream>
#include <string>
#include <string_view>
#include <utility>

#include "HttpParse.h"
#include "MysqlMgr.h"
#include "RedisMgr.h"
#include "StatusGrpcClient.h"
#include "VerifyGrpcClient.h"

namespace
{

using drogon::HttpRequestPtr;
using drogon::HttpResponsePtr;
using Callback = std::function<void(const HttpResponsePtr&)>;

HttpResponsePtr MakeJsonResponse(const Json::Value& root)
{
    auto response = drogon::HttpResponse::newHttpResponse();
    response->setStatusCode(drogon::k200OK);
    response->setContentTypeString(std::string_view("text/json"));
    response->setBody(root.toStyledString());
    return response;
}

void ReplyJson(const Callback& callback, const Json::Value& root)
{
    callback(MakeJsonResponse(root));
}

void ReplyError(const Callback& callback, int error)
{
    Json::Value root;
    root["error"] = error;
    ReplyJson(callback, root);
}

// Matches the old connection-level catch: a handler that throws becomes a 500 carrying
// Error_Json, so a bad request never closes the socket without a body.
void ReplyServerError(const Callback& callback)
{
    Json::Value root;
    root["error"] = ErrorCodes::Error_Json;
    auto response = MakeJsonResponse(root);
    response->setStatusCode(drogon::k500InternalServerError);
    callback(response);
}

// Parses the body as a JSON object; replies Error_Json and returns false otherwise.
bool ReadJsonBody(const HttpRequestPtr& request, const Callback& callback, Json::Value& src_root)
{
    if (!ParseJsonObject(std::string(request->body()), src_root))
    {
        std::cout << "Failed to parse JSON data!" << std::endl;
        ReplyError(callback, ErrorCodes::Error_Json);
        return false;
    }
    return true;
}

// Checks the emailed code; replies VerifyExpired / VerifyCodeErr and returns false on mismatch.
bool CheckVerifyCode(const Callback& callback, const std::string& email, const std::string& code)
{
    std::string verify_code;
    if (!RedisMgr::GetInstance()->Get(CODEPREFIX + email, verify_code))
    {
        std::cout << " get verify code expired" << std::endl;
        ReplyError(callback, ErrorCodes::VerifyExpired);
        return false;
    }
    if (verify_code != code)
    {
        std::cout << " verify code error" << std::endl;
        ReplyError(callback, ErrorCodes::VerifyCodeErr);
        return false;
    }
    return true;
}

// Codes are single-use: once consumed they must not authorize a second register/reset.
void ConsumeVerifyCode(const std::string& email)
{
    if (!RedisMgr::GetInstance()->Del(CODEPREFIX + email))
    {
        std::cout << " failed to delete verify code of " << email << std::endl;
    }
}

} // namespace

void LogicSystem::RegGet(const std::string& path, Handler handler)
{
    drogon::app().registerHandler(
        path,
        [handler = std::move(handler)](const HttpRequestPtr& request,
                                       std::function<void(const HttpResponsePtr&)>&& callback) {
            try
            {
                handler(request, callback);
            }
            catch (const std::exception& e)
            {
                std::cout << "handle request exception is " << e.what() << std::endl;
                ReplyServerError(callback);
            }
        },
        {drogon::Get});
}

void LogicSystem::RegPost(const std::string& path, Handler handler)
{
    drogon::app().registerHandler(
        path,
        [handler = std::move(handler)](const HttpRequestPtr& request,
                                       std::function<void(const HttpResponsePtr&)>&& callback) {
            try
            {
                handler(request, callback);
            }
            catch (const std::exception& e)
            {
                std::cout << "handle request exception is " << e.what() << std::endl;
                ReplyServerError(callback);
            }
        },
        {drogon::Post});
}

void LogicSystem::RegisterHandlers()
{
    RegGet("/get_test", [](const HttpRequestPtr& request, const Callback& callback) {
        std::string body = "receive get_test req";
        int i = 0;
        for (const auto& elem : request->getParameters())
        {
            ++i;
            body += " param " + std::to_string(i) + "key is" + elem.first;
            body += " param " + std::to_string(i) + "val is" + elem.second + "\n";
        }

        auto response = drogon::HttpResponse::newHttpResponse();
        response->setStatusCode(drogon::k200OK);
        response->setContentTypeString(std::string_view("text/plain"));
        response->setBody(std::move(body));
        callback(response);
    });

    RegPost("/get_verify_code", [](const HttpRequestPtr& request, const Callback& callback) {
        std::cout << "get_verify_code" << std::endl;
        Json::Value src_root;
        if (!ReadJsonBody(request, callback, src_root))
        {
            return;
        }

        std::string email;
        if (!GetStringField(src_root, "email", email, kMaxEmailLength))
        {
            ReplyError(callback, ErrorCodes::Error_Json);
            return;
        }

        std::cout << "email is " << email << std::endl;
        GetVerifyRsp rsp = VerifyGrpcClient::GetInstance()->GetVerifyCode(email);
        Json::Value root;
        root["error"] = rsp.error();
        root["email"] = email;
        ReplyJson(callback, root);
    });

    RegPost("/user_register", [](const HttpRequestPtr& request, const Callback& callback) {
        std::cout << "user_register" << std::endl;
        Json::Value src_root;
        if (!ReadJsonBody(request, callback, src_root))
        {
            return;
        }

        std::string email, name, pwd, confirm, icon, code;
        if (!GetStringField(src_root, "email", email, kMaxEmailLength) || !GetStringField(src_root, "user", name, kMaxFieldLength) || !GetStringField(src_root, "passwd", pwd, kMaxFieldLength) || !GetStringField(src_root, "confirm", confirm, kMaxFieldLength) || !GetStringField(src_root, "icon", icon, kMaxFieldLength) || !GetStringField(src_root, "verifycode", code, kMaxFieldLength))
        {
            ReplyError(callback, ErrorCodes::Error_Json);
            return;
        }

        if (pwd != confirm)
        {
            std::cout << "password err " << std::endl;
            ReplyError(callback, ErrorCodes::PasswdErr);
            return;
        }

        if (!CheckVerifyCode(callback, email, code))
        {
            return;
        }

        int uid = MysqlMgr::GetInstance()->RegUser(name, email, pwd, icon);
        if (uid == 0)
        {
            std::cout << " user or email exist" << std::endl;
            ReplyError(callback, ErrorCodes::UserExist);
            return;
        }
        if (uid < 0)
        {
            std::cout << " register user db error" << std::endl;
            ReplyError(callback, ErrorCodes::DbErr);
            return;
        }
        ConsumeVerifyCode(email);

        Json::Value root;
        root["error"] = 0;
        root["uid"] = uid;
        root["email"] = email;
        root["user"] = name;
        root["icon"] = icon;
        ReplyJson(callback, root);
    });

    RegPost("/reset_pwd", [](const HttpRequestPtr& request, const Callback& callback) {
        std::cout << "reset_pwd" << std::endl;
        Json::Value src_root;
        if (!ReadJsonBody(request, callback, src_root))
        {
            return;
        }

        std::string email, name, pwd, code;
        if (!GetStringField(src_root, "email", email, kMaxEmailLength) || !GetStringField(src_root, "user", name, kMaxFieldLength) || !GetStringField(src_root, "passwd", pwd, kMaxFieldLength) || !GetStringField(src_root, "verifycode", code, kMaxFieldLength))
        {
            ReplyError(callback, ErrorCodes::Error_Json);
            return;
        }

        if (!CheckVerifyCode(callback, email, code))
        {
            return;
        }

        DbStatus email_status = MysqlMgr::GetInstance()->CheckEmail(name, email);
        if (email_status == DbStatus::Error)
        {
            ReplyError(callback, ErrorCodes::DbErr);
            return;
        }
        if (email_status == DbStatus::Mismatch)
        {
            std::cout << " user email not match" << std::endl;
            ReplyError(callback, ErrorCodes::EmailNotMatch);
            return;
        }

        DbStatus up_status = MysqlMgr::GetInstance()->UpdatePwd(name, pwd);
        if (up_status == DbStatus::Error)
        {
            ReplyError(callback, ErrorCodes::DbErr);
            return;
        }
        if (up_status == DbStatus::Mismatch)
        {
            std::cout << " update pwd failed" << std::endl;
            ReplyError(callback, ErrorCodes::PasswdUpFailed);
            return;
        }
        ConsumeVerifyCode(email);

        std::cout << "succeed to update password of " << name << std::endl;
        Json::Value root;
        root["error"] = 0;
        root["email"] = email;
        root["user"] = name;
        ReplyJson(callback, root);
    });

    RegPost("/user_login", [](const HttpRequestPtr& request, const Callback& callback) {
        std::cout << "receive user login message" << std::endl;
        Json::Value src_root;
        if (!ReadJsonBody(request, callback, src_root))
        {
            return;
        }

        std::string email, pwd;
        if (!GetStringField(src_root, "email", email, kMaxEmailLength) || !GetStringField(src_root, "passwd", pwd, kMaxFieldLength))
        {
            ReplyError(callback, ErrorCodes::Error_Json);
            return;
        }
        std::cout << "login email is " << email << std::endl;

        UserInfo userInfo;
        DbStatus pwd_status = MysqlMgr::GetInstance()->CheckPwd(email, pwd, userInfo);
        if (pwd_status == DbStatus::Error)
        {
            ReplyError(callback, ErrorCodes::DbErr);
            return;
        }
        if (pwd_status == DbStatus::Mismatch)
        {
            std::cout << " user pwd not match" << std::endl;
            ReplyError(callback, ErrorCodes::PasswdInvalid);
            return;
        }

        // query StatusServer to find a suitable connection
        auto reply = StatusGrpcClient::GetInstance()->GetChatServer(userInfo.uid);
        if (reply.error())
        {
            std::cout << " grpc get chat server failed, error is " << reply.error() << std::endl;
            ReplyError(callback, ErrorCodes::RPCFailed);
            return;
        }

        std::cout << "succeed to load userinfo uid is " << userInfo.uid << std::endl;
        Json::Value root;
        root["error"] = 0;
        root["email"] = email;
        root["uid"] = userInfo.uid;
        root["token"] = reply.token();
        root["host"] = reply.host();
        root["port"] = reply.port();
        ReplyJson(callback, root);
    });
}

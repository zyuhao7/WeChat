#include "LogicSystem.h"
#include "HttpConnection.h"
#include "HttpParse.h"
#include "VerifyGrpcClient.h"
#include "RedisMgr.h"
#include "MysqlMgr.h"
#include "StatusGrpcClient.h"

namespace {

void ReplyJson(const std::shared_ptr<HttpConnection>& connection, const Json::Value& root)
{
	connection->AppendBody(root.toStyledString());
}

void ReplyError(const std::shared_ptr<HttpConnection>& connection, int error)
{
	Json::Value root;
	root["error"] = error;
	ReplyJson(connection, root);
}

// Parses the body as a JSON object; replies Error_Json and returns false otherwise.
bool ReadJsonBody(const std::shared_ptr<HttpConnection>& connection, Json::Value& src_root)
{
	connection->SetContentType("text/json");
	auto body_str = connection->RequestBody();
	if (!ParseJsonObject(body_str, src_root)) {
		std::cout << "Failed to parse JSON data!" << std::endl;
		ReplyError(connection, ErrorCodes::Error_Json);
		return false;
	}
	return true;
}

// Checks the emailed code; replies VerifyExpired / VerifyCodeErr and returns false on mismatch.
bool CheckVerifyCode(const std::shared_ptr<HttpConnection>& connection, const std::string& email,
	const std::string& code)
{
	std::string verify_code;
	if (!RedisMgr::GetInstance()->Get(CODEPREFIX + email, verify_code)) {
		std::cout << " get verify code expired" << std::endl;
		ReplyError(connection, ErrorCodes::VerifyExpired);
		return false;
	}
	if (verify_code != code) {
		std::cout << " verify code error" << std::endl;
		ReplyError(connection, ErrorCodes::VerifyCodeErr);
		return false;
	}
	return true;
}

// Codes are single-use: once consumed they must not authorize a second register/reset.
void ConsumeVerifyCode(const std::string& email)
{
	if (!RedisMgr::GetInstance()->Del(CODEPREFIX + email)) {
		std::cout << " failed to delete verify code of " << email << std::endl;
	}
}

}

void LogicSystem::RegGet(std::string url, HttpHandler handler)
{
	_get_handlers.insert(make_pair(url, handler));
}

void LogicSystem::RegPost(std::string url, HttpHandler handler)
{
	_post_handlers.insert(make_pair(url, handler));
}

LogicSystem::LogicSystem()
{
	RegGet("/get_test", [](std::shared_ptr<HttpConnection> connection) {
		beast::ostream(connection->_response.body()) << "receive get_test req";
		int i = 0;
		for (auto& elem : connection->_get_params)
		{
			i++;
			beast::ostream(connection->_response.body()) <<" param " << i << "key is" << elem.first;
			beast::ostream(connection->_response.body()) << " param " << i << "val is" << elem.second << std::endl;
		}
	});

    RegPost("/get_verify_code", [](std::shared_ptr<HttpConnection> connection) {
        std::cout << "get_verify_code" << std::endl;
        Json::Value src_root;
        if (!ReadJsonBody(connection, src_root)) {
            return;
        }

        std::string email;
        if (!GetStringField(src_root, "email", email, kMaxEmailLength)) {
            ReplyError(connection, ErrorCodes::Error_Json);
            return;
        }

        std::cout << "email is " << email << std::endl;
        GetVerifyRsp rsp = VerifyGrpcClient::GetInstance()->GetVerifyCode(email);
        Json::Value root;
        root["error"] = rsp.error();
        root["email"] = email;
        ReplyJson(connection, root);
        });

    RegPost("/user_register", [](std::shared_ptr<HttpConnection> connection) {
        std::cout << "user_register" << std::endl;
        Json::Value src_root;
        if (!ReadJsonBody(connection, src_root)) {
            return;
        }

        std::string email, name, pwd, confirm, icon, code;
        if (!GetStringField(src_root, "email", email, kMaxEmailLength)
            || !GetStringField(src_root, "user", name, kMaxFieldLength)
            || !GetStringField(src_root, "passwd", pwd, kMaxFieldLength)
            || !GetStringField(src_root, "confirm", confirm, kMaxFieldLength)
            || !GetStringField(src_root, "icon", icon, kMaxFieldLength)
            || !GetStringField(src_root, "verifycode", code, kMaxFieldLength)) {
            ReplyError(connection, ErrorCodes::Error_Json);
            return;
        }

        if (pwd != confirm) {
            std::cout << "password err " << std::endl;
            ReplyError(connection, ErrorCodes::PasswdErr);
            return;
        }

        if (!CheckVerifyCode(connection, email, code)) {
            return;
        }

        int uid = MysqlMgr::GetInstance()->RegUser(name, email, pwd, icon);
        if (uid == 0) {
            std::cout << " user or email exist" << std::endl;
            ReplyError(connection, ErrorCodes::UserExist);
            return;
        }
        if (uid < 0) {
            std::cout << " register user db error" << std::endl;
            ReplyError(connection, ErrorCodes::DbErr);
            return;
        }
        ConsumeVerifyCode(email);

        Json::Value root;
        root["error"] = 0;
        root["uid"] = uid;
        root["email"] = email;
        root["user"] = name;
        root["icon"] = icon;
        ReplyJson(connection, root);
        });

    RegPost("/reset_pwd", [](std::shared_ptr<HttpConnection> connection) {
        std::cout << "reset_pwd" << std::endl;
        Json::Value src_root;
        if (!ReadJsonBody(connection, src_root)) {
            return;
        }

        std::string email, name, pwd, code;
        if (!GetStringField(src_root, "email", email, kMaxEmailLength)
            || !GetStringField(src_root, "user", name, kMaxFieldLength)
            || !GetStringField(src_root, "passwd", pwd, kMaxFieldLength)
            || !GetStringField(src_root, "verifycode", code, kMaxFieldLength)) {
            ReplyError(connection, ErrorCodes::Error_Json);
            return;
        }

        if (!CheckVerifyCode(connection, email, code)) {
            return;
        }

        DbStatus email_status = MysqlMgr::GetInstance()->CheckEmail(name, email);
        if (email_status == DbStatus::Error) {
            ReplyError(connection, ErrorCodes::DbErr);
            return;
        }
        if (email_status == DbStatus::Mismatch) {
            std::cout << " user email not match" << std::endl;
            ReplyError(connection, ErrorCodes::EmailNotMatch);
            return;
        }

        DbStatus up_status = MysqlMgr::GetInstance()->UpdatePwd(name, pwd);
        if (up_status == DbStatus::Error) {
            ReplyError(connection, ErrorCodes::DbErr);
            return;
        }
        if (up_status == DbStatus::Mismatch) {
            std::cout << " update pwd failed" << std::endl;
            ReplyError(connection, ErrorCodes::PasswdUpFailed);
            return;
        }
        ConsumeVerifyCode(email);

        std::cout << "succeed to update password of " << name << std::endl;
        Json::Value root;
        root["error"] = 0;
        root["email"] = email;
        root["user"] = name;
        ReplyJson(connection, root);
        });

    RegPost("/user_login", [](std::shared_ptr<HttpConnection> connection) {
        std::cout << "receive user login message" << std::endl;
        Json::Value src_root;
        if (!ReadJsonBody(connection, src_root)) {
            return;
        }

        std::string email, pwd;
        if (!GetStringField(src_root, "email", email, kMaxEmailLength)
            || !GetStringField(src_root, "passwd", pwd, kMaxFieldLength)) {
            ReplyError(connection, ErrorCodes::Error_Json);
            return;
        }
        std::cout << "login email is " << email << std::endl;

        UserInfo userInfo;
        DbStatus pwd_status = MysqlMgr::GetInstance()->CheckPwd(email, pwd, userInfo);
        if (pwd_status == DbStatus::Error) {
            ReplyError(connection, ErrorCodes::DbErr);
            return;
        }
        if (pwd_status == DbStatus::Mismatch) {
            std::cout << " user pwd not match" << std::endl;
            ReplyError(connection, ErrorCodes::PasswdInvalid);
            return;
        }

        //query StatusServer to find a suitable connection
        auto reply = StatusGrpcClient::GetInstance()->GetChatServer(userInfo.uid);
        if (reply.error()) {
            std::cout << " grpc get chat server failed, error is " << reply.error() << std::endl;
            ReplyError(connection, ErrorCodes::RPCFailed);
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
        ReplyJson(connection, root);
        });
}

bool LogicSystem::HandleGet(std::string path, std::shared_ptr<HttpConnection> con)
{
	auto it = _get_handlers.find(path);
	if (it == _get_handlers.end())
	{
		return false;
	}
	it->second(con);
	return true;
}

bool LogicSystem::HandlePost(std::string path, std::shared_ptr<HttpConnection> con)
{
	auto it = _post_handlers.find(path);
	if (it == _post_handlers.end())
	{
        std::cout << "handle Post failed" << std::endl;
		return false;
	}
	it->second(con);
	return true;
}
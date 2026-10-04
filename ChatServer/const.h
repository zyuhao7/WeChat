#pragma once
#include <jdbc/cppconn/exception.h>
#include <jdbc/cppconn/prepared_statement.h>
#include <jdbc/cppconn/resultset.h>
#include <jdbc/cppconn/statement.h>
#include <jdbc/mysql_connection.h>
#include <jdbc/mysql_driver.h>
#include <json/json.h>
#include <json/reader.h>
#include <json/value.h>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/http.hpp>
#include <boost/filesystem.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <cassert>
#include <functional>

enum ErrorCodes
{
	Success = 0,
	Error_Json = 1001,		//JSON parse error
	RPCFailed = 1002,		//RPC request error
	VerifyExpired = 1003,   //verify code expired
	VerifyCodeErr = 1004,   //verify code wrong
	UserExist = 1005,       //the user already exists
	PasswdErr = 1006,       //wrong password
	EmailNotMatch = 1007,   //email mismatch
	PasswdUpFailed = 1008,  //password update failed
	PasswdInvalid = 1009,   //password update failed
	TokenInvalid = 1010,    //Token invalid
	UidInvalid = 1011,		//invalid uid
};

// Defer class
class Defer {
public:
	// accept a lambda or a function pointer
	Defer(std::function<void()> func) :func_(func)
	{
	}
	~Defer() {
		func_();
	}
private:
	std::function<void()> func_;
};

#define MAX_LENGTH  1024 * 2

#define HEAD_TOTAL_LEN 4  //total header length
#define HEAD_ID_LEN 2   //header id length
#define HEAD_DATA_LEN 2  //header data length

#define MAX_RECVQUE  10000
#define MAX_SENDQUE 1000


enum MSG_IDS {
	MSG_CHAT_LOGIN = 1005, //user login
	MSG_CHAT_LOGIN_RSP = 1006, //user login reply
	ID_SEARCH_USER_REQ = 1007, //user search request
	ID_SEARCH_USER_RSP = 1008, //search user reply
	ID_ADD_FRIEND_REQ = 1009, //add-friend apply request
	ID_ADD_FRIEND_RSP = 1010, //add-friend apply reply
	ID_NOTIFY_ADD_FRIEND_REQ = 1011,  //notify the user of the add-friend apply
	ID_AUTH_FRIEND_REQ = 1013,  //auth friend request
	ID_AUTH_FRIEND_RSP = 1014,  //auth friend reply
	ID_NOTIFY_AUTH_FRIEND_REQ = 1015, //notify the user of the friend auth apply
	ID_TEXT_CHAT_MSG_REQ = 1017, //text chat message request
	ID_TEXT_CHAT_MSG_RSP = 1018, //text chat message reply
	ID_NOTIFY_TEXT_CHAT_MSG_REQ = 1019, //notify the user of the text chat message
	ID_NOTIFY_OFF_LINE_REQ = 1021, //notify the user of going offline
	ID_HEART_BEAT_REQ = 1023,      //heartbeat request
	ID_HEARTBEAT_RSP = 1024,       //heartbeat reply
};

#define USERIPPREFIX  "uip_" 
#define USERTOKENPREFIX  "utoken_"
#define IPCOUNTPREFIX  "ipcount_"
#define USER_BASE_INFO "ubaseinfo_"
#define LOGIN_COUNT  "logincount"
#define NAME_INFO  "nameinfo_"

#define LOCK_PREFIX "lock_"
#define USER_SESSION_PREFIX "usession_"
#define LOCK_COUNT "lockcount"

// distributed lock lease time
#define LOCK_TIME_OUT 10
//distributed lock retry interval
#define ACQUIRE_TIME_OUT 5
#pragma once
#include <boost/beast/http.hpp>
#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <memory>
#include <iostream>
#include <unordered_map>
#include <json/json.h>
#include <json/value.h>
#include <json/reader.h>
#include "Singleton.h"
#include <assert.h>
#include <queue>
#include <jdbc/mysql_driver.h>
#include <jdbc/mysql_connection.h>
#include <jdbc/cppconn/prepared_statement.h>
#include <jdbc/cppconn/resultset.h>
#include <jdbc/cppconn/statement.h>
#include <jdbc/cppconn/exception.h>
#include <iostream>
#include <functional>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <string>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

enum ErrorCodes {
	Success = 0,
	Error_Json = 1001,  //JSON parse error
	RPCFailed = 1002,  //RPC request error
	VarifyExpired = 1003, //verify code expired
	VarifyCodeErr = 1004, //verify code wrong
	UserExist = 1005,       //the user already exists
	PasswdErr = 1006,    //wrong password
	EmailNotMatch = 1007,  //email mismatch
	PasswdUpFailed = 1008,  //password update failed
	PasswdInvalid = 1009,   //password update failed
	TokenInvalid = 1010,   //Token invalid
	UidInvalid = 1011,  //invalid uid
};


// Defer class
class Defer {
public:
	// accept a lambda or a function pointer
	Defer(std::function<void()> func) : func_(func) {}

	// run the passed function in the destructor
	~Defer() {
		func_();
	}

private:
	std::function<void()> func_;
};

#define USERIPPREFIX  "uip_"
#define USERTOKENPREFIX  "utoken_"
#define IPCOUNTPREFIX  "ipcount_"
#define USER_BASE_INFO "ubaseinfo_"
#define LOGIN_COUNT  "logincount"

#define LOCK_COUNT "lockcount"

// distributed lock lease time
#define LOCK_TIME_OUT 10
//distributed lock retry interval
#define ACQUIRE_TIME_OUT 5
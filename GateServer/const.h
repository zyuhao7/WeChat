#pragma once
#include <boost/beast/http.hpp>
#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include<functional>
#include <json/json.h>
#include <json/value.h>
#include <json/reader.h>
#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include<cassert>
#include<jdbc/mysql_connection.h>
#include<jdbc/mysql_driver.h>
#include<jdbc/cppconn/prepared_statement.h>
#include<jdbc/cppconn/resultset.h>
#include<jdbc/cppconn/statement.h>
#include<jdbc/cppconn/exception.h>
//#include<grpcpp/grpcpp.h>
#include<boost/uuid/uuid.hpp>
#include<boost/uuid/uuid_generators.hpp>
#include<boost/uuid/uuid_io.hpp>


namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>
 
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
	{}
	~Defer() {
		func_();
	}
private:
	std::function<void()> func_;
};


#define CODEPREFIX  "code_"


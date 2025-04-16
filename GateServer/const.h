#pragma once
#include <boost/beast/http.hpp>
#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <memory>
#include<iostream>
#include<string>
#include<functional>
#include <map>
#include <unordered_map>
#include <json/json.h>
#include <json/value.h>
#include <json/reader.h>
#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include<atomic>
#include<queue>
#include<mutex>
#include<condition_variable>
#include<cassert>
#include<thread>
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

#include "hiredis.h"
#include "Singleton.h"

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>
 
enum ErrorCodes
{
	Success = 0,
	Error_Json = 1001,		//Json��������
	RPCFailed = 1002,		//RPC�������
	VerifyExpired = 1003,   //��֤�����
	VerifyCodeErr = 1004,   //��֤�����
	UserExist = 1005,       //�û��Ѿ�����
	PasswdErr = 1006,       //�������
	EmailNotMatch = 1007,   //���䲻ƥ��
	PasswdUpFailed = 1008,  //��������ʧ��
	PasswdInvalid = 1009,   //�������ʧ��
	TokenInvalid = 1010,    //TokenʧЧ
	UidInvalid = 1011,		//uid��Ч
};

// Defer ��
class Defer {
public:
		// ����һ��lambda���ʽ���ߺ���ָ��
	Defer(std::function<void()> func) :func_(func)
	{}
	~Defer() {
		func_();
	}
private:
	std::function<void()> func_;
};


#define CODEPREFIX  "code_"


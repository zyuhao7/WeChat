#pragma once
#include <boost/beast/http.hpp>
#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <memory>
#include<iostream>
#include <vector>
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
 
enum ErrorCodes
{
	Success = 0,
	Error_Json = 1001,		//Json解析错误
	RPCFailed = 1002,		//RPC请求错误
	VerifyExpired = 1003,   //验证码过期
	VerifyCodeErr = 1004,   //验证码错误
	UserExist = 1005,       //用户已经存在
	PasswdErr = 1006,       //密码错误
	EmailNotMatch = 1007,   //邮箱不匹配
	PasswdUpFailed = 1008,  //更新密码失败
	PasswdInvalid = 1009,   //密码更新失败
	TokenInvalid = 1010,    //Token失效
	UidInvalid = 1011,		//uid无效
};

// Defer 类
class Defer {
public:
	// 接受一个lambda表达式或者函数指针
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
//头部总长度
#define HEAD_TOTAL_LEN 4
//头部id长度
#define HEAD_ID_LEN 2
//头部数据长度
#define HEAD_DATA_LEN 2

#define MAX_RECVQUE  10000
#define MAX_SENDQUE 1000


enum MSG_IDS {
	MSG_CHAT_LOGIN = 1005, //用户登陆
	MSG_CHAT_LOGIN_RSP = 1006, //用户登陆回包
	ID_SEARCH_USER_REQ = 1007, //用户搜索请求
	ID_SEARCH_USER_RSP = 1008, //搜索用户回包
	ID_ADD_FRIEND_REQ = 1009, //申请添加好友请求
	ID_ADD_FRIEND_RSP = 1010, //申请添加好友回复
	ID_NOTIFY_ADD_FRIEND_REQ = 1011,  //通知用户添加好友申请
	ID_AUTH_FRIEND_REQ = 1013,  //认证好友请求
	ID_AUTH_FRIEND_RSP = 1014,  //认证好友回复
	ID_NOTIFY_AUTH_FRIEND_REQ = 1015, //通知用户认证好友申请
	ID_TEXT_CHAT_MSG_REQ = 1017, //文本聊天信息请求
	ID_TEXT_CHAT_MSG_RSP = 1018, //文本聊天信息回复
	ID_NOTIFY_TEXT_CHAT_MSG_REQ = 1019, //通知用户文本聊天信息
};

#define USERIPPREFIX  "uip_" 
#define USERTOKENPREFIX  "utoken_"
#define IPCOUNTPREFIX  "ipcount_"
#define USER_BASE_INFO "ubaseinfo_"
#define LOGIN_COUNT  "logincount"
#define NAME_INFO  "nameinfo_"


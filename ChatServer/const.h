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
	{
	}
	~Defer() {
		func_();
	}
private:
	std::function<void()> func_;
};

#define MAX_LENGTH  1024 * 2
//ͷ���ܳ���
#define HEAD_TOTAL_LEN 4
//ͷ��id����
#define HEAD_ID_LEN 2
//ͷ�����ݳ���
#define HEAD_DATA_LEN 2

#define MAX_RECVQUE  10000
#define MAX_SENDQUE 1000


enum MSG_IDS {
	MSG_CHAT_LOGIN = 1005, //�û���½
	MSG_CHAT_LOGIN_RSP = 1006, //�û���½�ذ�
	ID_SEARCH_USER_REQ = 1007, //�û���������
	ID_SEARCH_USER_RSP = 1008, //�����û��ذ�
	ID_ADD_FRIEND_REQ = 1009, //������Ӻ�������
	ID_ADD_FRIEND_RSP = 1010, //������Ӻ��ѻظ�
	ID_NOTIFY_ADD_FRIEND_REQ = 1011,  //֪ͨ�û���Ӻ�������
	ID_AUTH_FRIEND_REQ = 1013,  //��֤��������
	ID_AUTH_FRIEND_RSP = 1014,  //��֤���ѻظ�
	ID_NOTIFY_AUTH_FRIEND_REQ = 1015, //֪ͨ�û���֤��������
	ID_TEXT_CHAT_MSG_REQ = 1017, //�ı�������Ϣ����
	ID_TEXT_CHAT_MSG_RSP = 1018, //�ı�������Ϣ�ظ�
	ID_NOTIFY_TEXT_CHAT_MSG_REQ = 1019, //֪ͨ�û��ı�������Ϣ
};

#define USERIPPREFIX  "uip_" 
#define USERTOKENPREFIX  "utoken_"
#define IPCOUNTPREFIX  "ipcount_"
#define USER_BASE_INFO "ubaseinfo_"
#define LOGIN_COUNT  "logincount"
#define NAME_INFO  "nameinfo_"


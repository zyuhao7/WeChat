#include "LogicSystem.h"
#include <csignal>
#include <thread>
#include <mutex>
#include "AsioIOServicePool.h"
#include "CServer.h"
#include "ConfigMgr.h"
#include "RedisMgr.h"
#include "ChatServiceImpl.h"

using namespace std;
bool bstop = false;
std::condition_variable cond_quit;
std::mutex mutex_quit;

#include "DistLock.h"
#include <iostream>
#include <thread>
#include <string>
#include <chrono>
#include <windows.h>
#include <hiredis.h>
using namespace std;

int TestDisLock() {
    // 连接到 Redis 服务器（根据实际情况修改主机和端口）
    redisContext* context = redisConnect("81.68.86.146", 6380);
    if (context == nullptr || context->err) {
        if (context) {
            std::cerr << "连接错误: " << context->errstr << std::endl;
            redisFree(context);
        }
        else {
            std::cerr << "无法分配 redis context" << std::endl;
        }
        return 1;
    }

    std::string redis_password = "123456";
    redisReply* r = (redisReply*)redisCommand(context, "AUTH %s", redis_password.c_str());
    if (r->type == REDIS_REPLY_ERROR) {
        printf("Redis认证失败！\n");
    }
    else {
        printf("Redis认证成功！\n");
    }

    // 尝试获取锁（锁有效期 10 秒，获取超时时间 5 秒）
    std::string lockId = DistLock::Inst().acquireLock(context, "my_resource", 10, 5);

    if (!lockId.empty()) {
        std::cout << "子进程 " << GetCurrentProcessId() << " 成功获取锁，锁 ID: " << lockId << std::endl;
        // 执行需要保护的临界区代码
        std::this_thread::sleep_for(std::chrono::seconds(2));

        // 释放锁
        if (DistLock::Inst().releaseLock(context, "my_resource", lockId)) {
            std::cout << "子进程 " << GetCurrentProcessId() << " 成功释放锁" << std::endl;
        }
        else {
            std::cout << "子进程 " << GetCurrentProcessId() << " 释放锁失败" << std::endl;
        }
    }
    else {
        std::cout << "子进程 " << GetCurrentProcessId() << " 获取锁失败" << std::endl;
    }

    // 释放 Redis 连接
    redisFree(context);
}


int main() {

    TestDisLock();

}

//int main()
//{
//
//	auto& cfg = ConfigMgr::Inst();
//	auto server_name = cfg["SelfServer"]["Name"];
//	try {
//		auto pool = AsioIOServicePool::GetInstance();
//		//将登录数设置为0
//		RedisMgr::GetInstance()->HSet(LOGIN_COUNT, server_name, "0");
//
//		//定义一个GrpcServer
//		std::string server_address(cfg["SelfServer"]["Host"] + ":" + cfg["SelfServer"]["RPCPort"]);
//		ChatServiceImpl service;
//		grpc::ServerBuilder builder;
//		// 监听端口和添加服务
//		builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
//		builder.RegisterService(&service);
//		// 构建并启动gRPC服务器
//		std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
//		std::cout << "RPC Server listening on " << server_address << std::endl;
//
//		//单独启动一个线程处理grpc服务
//		std::thread  grpc_server_thread([&server]() {
//			server->Wait();
//			});
//
//		boost::asio::io_context  io_context;
//		boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);
//		signals.async_wait([&io_context, pool, &server](auto, auto) {
//			io_context.stop();
//			pool->Stop();
//			server->Shutdown();
//			});
//
//		auto port_str = cfg["SelfServer"]["Port"];
//		CServer s(io_context, atoi(port_str.c_str()));
//		io_context.run();
//		RedisMgr::GetInstance()->HDel(LOGIN_COUNT, server_name);
//		RedisMgr::GetInstance()->Close();
//		grpc_server_thread.join();
//	}
//	catch (std::exception& e) {
//		std::cerr << "Exception: " << e.what() << endl;
//		RedisMgr::GetInstance()->HDel(LOGIN_COUNT, server_name);
//		RedisMgr::GetInstance()->Close();
//	}
//
//}

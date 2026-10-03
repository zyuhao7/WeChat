#include "LogicSystem.h"
#include <csignal>
#include <thread>
#include <mutex>
#include "AsioIOServicePool.h"
#include "CServer.h"
#include "ConfigMgr.h"
#include "RedisMgr.h"
#include "ChatServiceImpl.h"
#include "const.h"

using namespace std;
bool bstop = false;
std::condition_variable cond_quit;
std::mutex mutex_quit;

//#include "DistLock.h"
//#include <iostream>
//#include <chrono>
//#include <windows.h>
//#include <hiredis.h>
//using namespace std;
//
//int TestDisLock() {
//    // connect to the Redis server (adjust host and port as needed)
//    redisContext* context = redisConnect("81.68.86.146", 6380);
//    if (context == nullptr || context->err) {
//        if (context) {
//            std::cerr << "connection error: " << context->errstr << std::endl;
//            redisFree(context);
//        }
//        else {
//            std::cerr << "failed to allocate redis context" << std::endl;
//        }
//        return 1;
//    }
//
//    std::string redis_password = "123456";
//    redisReply* r = (redisReply*)redisCommand(context, "AUTH %s", redis_password.c_str());
//    if (r->type == REDIS_REPLY_ERROR) {
//        printf("Redis auth failed!\n");
//    }
//    else {
//        printf("Redis auth succeeded!\n");
//    }
//
//    // try to acquire the lock (10s lease, 5s acquire timeout)
//    std::string lockId = DistLock::Inst().acquireLock(context, "my_resource", 10, 5);
//
//    if (!lockId.empty()) {
//        std::cout << "child process " << GetCurrentProcessId() << " acquired the lock, lock ID: " << lockId << std::endl;
//        // execute the critical section that needs protection
//        std::this_thread::sleep_for(std::chrono::seconds(2));
//
//        // release the lock
//        if (DistLock::Inst().releaseLock(context, "my_resource", lockId)) {
//            std::cout << "child process " << GetCurrentProcessId() << " released the lock" << std::endl;
//        }
//        else {
//            std::cout << "child process " << GetCurrentProcessId() << " failed to release the lock" << std::endl;
//        }
//    }
//    else {
//        std::cout << "child process " << GetCurrentProcessId() << " failed to acquire the lock" << std::endl;
//    }
//
//    // release the Redis connection
//    redisFree(context);
//}


//int main() {
//
//    TestDisLock();
//
//}

int main()
{

	auto& cfg = ConfigMgr::Inst();
	auto server_name = cfg["SelfServer"]["Name"];
	try {
		auto pool = AsioIOServicePool::GetInstance();
		//set the login count to 0
		RedisMgr::GetInstance()->HSet(LOGIN_COUNT, server_name, "0");
		Defer defer([server_name]() {
			RedisMgr::GetInstance()->HDel(LOGIN_COUNT, server_name);
			RedisMgr::GetInstance()->Close();
			});

		boost::asio::io_context io_context;
		auto port_str = cfg["SelfServer"]["Port"];
		auto pointer_server = std::make_shared<CServer>(io_context, atoi(port_str.c_str()));
		pointer_server->StartTimer();
			
		//define a GrpcServer
		std::string server_address(cfg["SelfServer"]["Host"] + ":" + cfg["SelfServer"]["RPCPort"]);
		ChatServiceImpl service;
		grpc::ServerBuilder builder;
		// listen on the port and add the service
		builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
		builder.RegisterService(&service);
		// build and start the gRPC server
		std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
		std::cout << "RPC Server listening on " << server_address << std::endl;

		//start a dedicated thread to run the grpc service
		std::thread  grpc_server_thread([&server]() {
			server->Wait();
			});

		boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);
		signals.async_wait([&io_context, pool, &server](auto, auto) {
			io_context.stop();
			pool->Stop();
			server->Shutdown();
			});

		LogicSystem::GetInstance()->SetServer(pointer_server);
		io_context.run();

		grpc_server_thread.join();
		pointer_server->StopTimer();
		return 0;
	}
	catch (std::exception& e) {
		std::cerr << "Exception: " << e.what() << endl;
	}
}

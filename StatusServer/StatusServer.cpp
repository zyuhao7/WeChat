#include <iostream>
#include <json/json.h>
#include <json/value.h>
#include <json/reader.h>
#include "const.h"
#include "ConfigMgr.h"
#include "hiredis.h"
#include "RedisMgr.h"
#include "MysqlMgr.h"
#include "AsioIOServicePool.h"
#include <memory>
#include <string>
#include <thread>
#include <boost/asio.hpp>
#include "StatusServiceImpl.h"
void RunServer() {
    auto& cfg = ConfigMgr::Inst();

    std::string server_address(cfg["StatusServer"]["Host"] + ":" + cfg["StatusServer"]["Port"]);
    StatusServiceImpl service;

    grpc::ServerBuilder builder;

    // listen on the port and add the service
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    // build and start the gRPC server
    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    std::cout << "Server listening on " << server_address << std::endl;

    // create the Boost.Asio io_context
    boost::asio::io_context io_context;
    // create a signal_set to catch SIGINT
    boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);

    // asynchronously wait for the SIGINT signal
    signals.async_wait([&server](const boost::system::error_code& error, int signal_number) {
        if (!error) {
            std::cout << "Shutting down server..." << std::endl;
            server->Shutdown(); // gracefully shut down the server
        }
        });

    // run the io_context on a separate thread
    std::thread([&io_context]() { io_context.run(); }).detach();

    // wait for the server to shut down
    server->Wait();
    io_context.stop(); // stop the io_context
}

int main(int argc, char** argv) {
    try 
    {
        RunServer();
    }
    catch (std::exception const& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return 0;
}
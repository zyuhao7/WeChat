#include <drogon/drogon.h>

#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>

#include "ConfigMgr.h"
#include "LogicSystem.h"
#include "MysqlMgr.h"
#include "RedisMgr.h"

int main()
{
    auto& gCfgMgr = ConfigMgr::Inst();
    std::string gate_port_str = gCfgMgr["GateServer"]["Port"];
    int gate_port = atoi(gate_port_str.c_str());
    if (gate_port <= 0 || gate_port > 65535)
    {
        std::cerr << "invalid [GateServer] Port '" << gate_port_str << "' in config.ini" << std::endl;
        return EXIT_FAILURE;
    }

    try
    {
        // connect eagerly so a missing Redis/MySQL shows up at startup, not on the first request
        RedisMgr::GetInstance();
        MysqlMgr::GetInstance();

        auto& app = drogon::app();
        app.addListener("0.0.0.0", static_cast<uint16_t>(gate_port));
        // handlers still call the synchronous Redis/MySQL/gRPC clients, so give the loop
        // enough threads that one slow dependency does not stall every connection
        app.setThreadNum(4);
        app.setServerHeaderField("GateServer");
        app.setLogLevel(trantor::Logger::kWarn);

        LogicSystem::GetInstance()->RegisterHandlers();

        auto not_found = drogon::HttpResponse::newHttpResponse();
        not_found->setStatusCode(drogon::k404NotFound);
        not_found->setContentTypeString(std::string_view("text/plain"));
        not_found->setBody("url not found\r\n");
        app.setCustom404Page(not_found);

        std::cout << "Gate Server listen on port: " << gate_port << std::endl;
        app.run();
    }
    catch (std::exception const& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}

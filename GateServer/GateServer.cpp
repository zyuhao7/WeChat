#include <iostream>
#include <json/json.h>
#include <json/value.h>
#include <json/reader.h>
#include "CServer.h"
#include "ConfigMgr.h"
#include "RedisMgr.h"
#include "MysqlMgr.h"

int main()
{
	auto& gCfgMgr = ConfigMgr::Inst();
	std::string gate_port_str = gCfgMgr["GateServer"]["Port"];
	int gate_port = atoi(gate_port_str.c_str());
	if (gate_port <= 0 || gate_port > 65535) {
		std::cerr << "invalid [GateServer] Port '" << gate_port_str << "' in config.ini" << std::endl;
		return EXIT_FAILURE;
	}

	try
	{
		// connect eagerly so a missing Redis/MySQL shows up at startup, not on the first request
		RedisMgr::GetInstance();
		MysqlMgr::GetInstance();

		unsigned short port = static_cast<unsigned short>(gate_port);
		net::io_context ioc{ 1 };
		boost::asio::signal_set signals(ioc, SIGINT, SIGTERM);
		signals.async_wait([&ioc](const boost::system::error_code& error, int signal_number) {

			if (error) {
				return;
			}
			ioc.stop();
			});
		std::make_shared<CServer>(ioc, port)->Start();
		std::cout << "Gate Server listen on port: " << port << std::endl;
		ioc.run();
	}
	catch (std::exception const& e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
		return EXIT_FAILURE;
	}
}
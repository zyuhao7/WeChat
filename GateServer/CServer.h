#pragma once
#include "const.h"
#include <string>

class CServer : public std::enable_shared_from_this<CServer>
{
public:
	CServer(boost::asio::io_context& ioc, unsigned short& port);
	void Start();

private:
	tcp::acceptor _acceptor;
	net::io_context& _ioc;
};


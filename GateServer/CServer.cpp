#include "CServer.h"
#include "HttpConnection.h"
#include "const.h"
#include "AsioIOServicePool.h"
#include <iostream>

CServer::CServer(boost::asio::io_context& ioc, unsigned short& port)
	:_ioc(ioc),
	_acceptor(ioc)
{
	// set SO_REUSEADDR before binding so a quick restart is not blocked by
	// lingering connections from the previous instance
	boost::system::error_code ec;
	_acceptor.open(tcp::v4(), ec);
	_acceptor.set_option(tcp::acceptor::reuse_address(true), ec);
	_acceptor.bind(tcp::endpoint(tcp::v4(), port), ec);
	_acceptor.listen(boost::asio::socket_base::max_listen_connections, ec);
}

void CServer::Start()
{
	auto self = shared_from_this();
	auto& io_context = AsioIOServicePool::GetInstance()->GetIOService();
	std::shared_ptr<HttpConnection> new_con = std::make_shared<HttpConnection>(io_context);

	_acceptor.async_accept(new_con->GetSocket(), [self, new_con](beast::error_code ec) {
		try{
			// on error drop the connection and keep accepting others
			if (ec)
			{
				self->Start();
				return;
			}
			// create a new connection and an HttpConnection to manage it
			//std::make_shared<HttpConnection>(std::move(self->_socket))->Start();
			new_con->Start();
			// 
			// keep listening
			self->Start();
		}
		catch (std::exception& ep)
		{
			std::cout << "exception is" << ep.what() << std::endl;
			self->Start();
		}
		});  
}


#include "CServer.h"
#include "HttpConnection.h"
#include "const.h"
#include "AsioIOServicePool.h"
#include <iostream>

CServer::CServer(boost::asio::io_context& ioc, unsigned short& port)
	:_ioc(ioc),
	_acceptor(ioc, tcp::endpoint(tcp::v4(),port))
{}

void CServer::Start()
{
	auto self = shared_from_this();
	auto& io_context = AsioIOServicePool::GetInstance()->GetIOService();
	std::shared_ptr<HttpConnection> new_con = std::make_shared<HttpConnection>(io_context);

	_acceptor.async_accept(new_con->GetSocket(), [self, new_con](beast::error_code ec) {
		try{
			// �����������, ����������������
			if (ec)
			{
				self->Start();
				return;
			}
			// ����������, ���Ҵ��� HttpConnection������������
			//std::make_shared<HttpConnection>(std::move(self->_socket))->Start();
			new_con->Start();
			// 
			// ��������
			self->Start();
		}
		catch (std::exception& ep)
		{
			std::cout << "exception is" << ep.what() << std::endl;
			self->Start();
		}
		});  
}


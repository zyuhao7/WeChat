#include "CServer.h"
#include <iostream>
#include "AsioIOServicePool.h"
#include "UserMgr.h"
#include "RedisMgr.h"
#include "ConfigMgr.h"

CServer::CServer(boost::asio::io_context& io_context, short port) :_io_context(io_context), _port(port),
		_acceptor(io_context, tcp::endpoint(tcp::v4(), port)),
		_timer(_io_context, std::chrono::seconds(60))
{
	cout << "Server start success, listen on port : " << _port << endl;
	StartAccept();
}

CServer::~CServer() {
	cout << "Server destruct listen on port : " << _port << endl;
}

void CServer::HandleAccept(shared_ptr<CSession> new_session, const boost::system::error_code& error) {
	if (!error) {
		new_session->Start();
		lock_guard<mutex> lock(_mutex);
		_sessions.insert(make_pair(new_session->GetSessionId(), new_session));
	}
	else {
		cout << "session accept failed, error is " << error.what() << endl;
	}

	StartAccept();
}

void CServer::StartAccept() {
	auto& io_context = AsioIOServicePool::GetInstance()->GetIOService();
	shared_ptr<CSession> new_session = make_shared<CSession>(io_context, this);
	_acceptor.async_accept(new_session->GetSocket(), std::bind(&CServer::HandleAccept, this, new_session, placeholders::_1));
}

// //根据session 的id删除 session，并移除用户和session的关联
void CServer::ClearSession(std::string session_id) {
	lock_guard<mutex> lock(_mutex);
	if (_sessions.find(session_id) != _sessions.end()) {
		auto uid = _sessions[session_id]->GetUserId();

		//移除用户和session的关联
		UserMgr::GetInstance()->RmvUserSession(uid, session_id);
	}
	_sessions.erase(session_id);

}

shared_ptr<CSession> CServer::GetSession(std::string uid)
{
	std::lock_guard<std::mutex> lock(_mutex);
	auto it = _sessions.find(uid);
	if (it != _sessions.end())
		return it->second;
	return nullptr;
}

bool CServer::CheckValid(std::string uid)
{
	std::lock_guard<std::mutex> lock(_mutex);
	auto it = _sessions.find(uid);
	if (it != _sessions.end())
		return true;
	return false;
}

void CServer::on_timer(const boost::system::error_code& ec)
{
	if (ec)
	{
		std::cout << "timer error: " << ec.message() << std::endl;
		return;
	}
	std::vector<std::shared_ptr<CSession>> _expired_sessions;
	int session_count = 0;
	// 加锁遍历
	std::map<std::string, shared_ptr<CSession>> sessions_copy;
	{
		lock_guard<mutex> lock(_mutex);
		sessions_copy = _sessions;
	}

	time_t now = std::time(nullptr);
	for (auto it = sessions_copy.begin(); it != sessions_copy.end(); it++)
	{
		auto b_expired = it->second->IsHeartbeatExpired(now);
		if (b_expired)
		{
			// 关闭 socket, 这里也会触发 async_read 的错误处理
			it->second->Close();
			// 收集过期信息
			_expired_sessions.push_back(it->second);
			continue;
		}
		session_count++;
	}

	// 设置 session的数量
	auto& cfg = ConfigMgr::Inst();
	auto self_name = cfg["SelfServer"]["Name"];
	auto count_str = std::to_string(session_count);
	RedisMgr::GetInstance()->HSet(LOGIN_COUNT, self_name, count_str);

	// 处理过期 session, 单独处理防止死锁
	for(auto& session : _expired_sessions)
	{
		session->DealExceptionSesseion();
	}

	// 再次设置下一个 60s 检测
	_timer.expires_after(std::chrono::seconds(60));
	_timer.async_wait([this](boost::system::error_code ec) {
		on_timer(ec);
		});
}

void CServer::StartTimer()
{
	//启动定时器
	auto self(shared_from_this());
	_timer.async_wait([self](boost::system::error_code ec) {
		self->on_timer(ec);
		});
}

void CServer::StopTimer()
{
	_timer.cancel();
}

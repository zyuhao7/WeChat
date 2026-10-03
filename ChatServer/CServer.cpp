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

// // remove the session by its id and unlink the user from the session
void CServer::ClearSession(std::string session_id) {
	lock_guard<mutex> lock(_mutex);
	if (_sessions.find(session_id) != _sessions.end()) {
		auto uid = _sessions[session_id]->GetUserId();

		//unlink the user from the session
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
	// lock and iterate
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
			// close the socket; this also triggers the async_read error handler
			it->second->Close();
			// collect expired info
			_expired_sessions.push_back(it->second);
			continue;
		}
		session_count++;
	}

	// set the session count
	auto& cfg = ConfigMgr::Inst();
	auto self_name = cfg["SelfServer"]["Name"];
	auto count_str = std::to_string(session_count);
	RedisMgr::GetInstance()->HSet(LOGIN_COUNT, self_name, count_str);

	// handle expired sessions separately to avoid deadlock
	for(auto& session : _expired_sessions)
	{
		session->DealExceptionSesseion();
	}

	// schedule the next 60s check again
	_timer.expires_after(std::chrono::seconds(60));
	_timer.async_wait([this](boost::system::error_code ec) {
		on_timer(ec);
		});
}

void CServer::StartTimer()
{
	//start the timer
	auto self(shared_from_this());
	_timer.async_wait([self](boost::system::error_code ec) {
		self->on_timer(ec);
		});
}

void CServer::StopTimer()
{
	_timer.cancel();
}

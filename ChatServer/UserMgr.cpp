#include "UserMgr.h"
#include "CSession.h"
#include "RedisMgr.h"

UserMgr::~UserMgr()
{
	_uid_to_session.clear();
}

std::shared_ptr<CSession> UserMgr::GetSession(int uid)
{
	std::lock_guard<std::mutex> lock(_session_mtx);
	auto it = _uid_to_session.find(uid);
	if (it == _uid_to_session.end())
		return nullptr;

	return it->second;
}

void UserMgr::SetUserSession(int uid, std::shared_ptr<CSession> session)
{
	std::lock_guard<std::mutex> lock(_session_mtx);
	_uid_to_session[uid] = session;
}

void UserMgr::RmvUserSession(int uid, const std::string& session_id)
{
	auto uid_str = std::to_string(uid);
	{
		std::lock_guard<std::mutex> lock(_session_mtx);
		auto it = _uid_to_session.find(uid);
		// 已被新会话顶替：不能删除当前有效映射
		if (it == _uid_to_session.end() || it->second->GetSessionId() != session_id)
			return;
		_uid_to_session.erase(it);
	}
	RedisMgr::GetInstance()->Del(USERIPPREFIX + uid_str);
}

UserMgr::UserMgr()
{}
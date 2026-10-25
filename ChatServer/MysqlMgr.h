#pragma once
#include <vector>
#include "MysqlDao.h"
#include "Singleton.h"
class MysqlMgr : public Singleton<MysqlMgr>
{
    friend class Singleton<MysqlMgr>;

public:
    ~MysqlMgr();
    int RegUser(const std::string& name, const std::string& email, const std::string& pwd);
    bool CheckEmail(const std::string& name, const std::string& email);
    bool UpdatePwd(const std::string& name, const std::string& pwd);
    bool CheckPwd(const std::string& name, const std::string& pwd, UserInfo& userInfo);
    bool AddFriendApply(const int& from, const int& to);
    bool AuthFriendApply(const int& from, const int& to);
    bool AddFriend(const int& from, const int& to, std::string back_name);
    std::shared_ptr<UserInfo> GetUser(int uid);
    std::shared_ptr<UserInfo> GetUser(std::string name);
    bool GetApplyList(int touid, std::vector<std::shared_ptr<ApplyInfo>>& applyList, int begin, int limit = 10);
    bool GetFriendList(int self_id, std::vector<std::shared_ptr<UserInfo>>& _info);
    bool AddChatMessages(int from_uid, int to_uid,
                         const std::vector<std::pair<std::string, std::string>>& messages);
    // before_id is an exclusive upper bound; 0 returns the newest page
    bool GetChatHistory(int self_uid, int peer_uid, long long before_id, int limit,
                        std::vector<std::shared_ptr<ChatMessageInfo>>& messages, bool& has_more);

private:
    MysqlMgr();
    MysqlDao _dao;
};
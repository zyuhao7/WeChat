#pragma once
#include "const.h"
#include "MysqlDao.h"
#include "Singleton.h"
class MysqlMgr : public Singleton<MysqlMgr>
{
    friend class Singleton<MysqlMgr>;

public:
    ~MysqlMgr();
    int RegUser(const std::string& name, const std::string& email, const std::string& pwd, const std::string& icon);
    DbStatus CheckEmail(const std::string& name, const std::string& email);
    DbStatus UpdatePwd(const std::string& name, const std::string& pwd);
    DbStatus CheckPwd(const std::string& name, const std::string& pwd, UserInfo& userInfo);

private:
    MysqlMgr();
    MysqlDao _dao;
};
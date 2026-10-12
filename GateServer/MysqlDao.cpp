#include "MysqlDao.h"
#include "ConfigMgr.h"

namespace
{

void LogSqlException(const sql::SQLException& e)
{
    std::cerr << "SQLException: " << e.what();
    std::cerr << " (MySQL error code: " << e.getErrorCode();
    std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
}

} // namespace

MysqlDao::MysqlDao()
{
    auto& cfg = ConfigMgr::Inst();
    const auto& host = cfg["Mysql"]["Host"];
    const auto& port = cfg["Mysql"]["Port"];
    const auto& pwd = cfg["Mysql"]["Passwd"];
    const auto& schema = cfg["Mysql"]["Schema"];
    const auto& user = cfg["Mysql"]["User"];
    pool_.reset(new MySqlPool(host + ":" + port, user, pwd, schema, 5));
}

MysqlDao::~MysqlDao()
{
    pool_->Close();
}

int MysqlDao::RegUserTransaction(const std::string& name, const std::string& email, const std::string& pwd, const std::string& icon)
{
    auto con = pool_->getConnection();
    Defer defer([this, &con]() {
        // restore autocommit so the pooled connection is not left in
        // manual-commit mode (which would poison the next borrower)
        if (con && con->_con)
        {
            try
            {
                con->_con->setAutoCommit(true);
            }
            catch (...)
            {
            }
        }
        pool_->returnConnection(std::move(con));
    });
    if (con == nullptr || con->_con == nullptr)
        return -1;

    try
    {
        // begin the transaction
        con->_con->setAutoCommit(false);

        // run the first DB query: find the user by email
        std::unique_ptr<sql::PreparedStatement> pstmt_email(con->_con->prepareStatement("select 1 from user where email = ?"));
        pstmt_email->setString(1, email);
        std::unique_ptr<sql::ResultSet> res_email(pstmt_email->executeQuery());
        if (res_email->next())
        {
            con->_con->rollback();
            std::cout << "email " << email << " exist" << std::endl;
            return 0;
        }

        // check whether the username is duplicated
        std::unique_ptr<sql::PreparedStatement> pstmt_name(con->_con->prepareStatement("select 1 from user where name = ?"));
        pstmt_name->setString(1, name);
        std::unique_ptr<sql::ResultSet> res_name(pstmt_name->executeQuery());
        if (res_name->next())
        {
            con->_con->rollback();
            std::cout << "name " << name << " exist" << std::endl;
            return 0;
        }

        // prepare to update the user id
        std::unique_ptr<sql::PreparedStatement> pstmt_upid(con->_con->prepareStatement("update user_id set id = id + 1"));
        pstmt_upid->executeUpdate();

        std::unique_ptr<sql::PreparedStatement> pstmt_uid(con->_con->prepareStatement("select id from user_id"));
        std::unique_ptr<sql::ResultSet> res_uid(pstmt_uid->executeQuery());
        int newId = 0;

        if (res_uid->next())
        {
            newId = res_uid->getInt("id");
        }
        else
        {
            std::cout << "select id from user_id failed " << std::endl;
            con->_con->rollback();
            return -1;
        }
        // insert user info
        std::unique_ptr<sql::PreparedStatement> pstmt_insert(con->_con->prepareStatement("insert into user(uid, name,email, pwd, nick, icon)"
                                                                                         "values(?,?,?,?,?,?)"));
        pstmt_insert->setInt(1, newId);
        pstmt_insert->setString(2, name);
        pstmt_insert->setString(3, email);
        pstmt_insert->setString(4, pwd);
        pstmt_insert->setString(5, name);
        pstmt_insert->setString(6, icon);
        pstmt_insert->executeUpdate();
        con->_con->commit();
        std::cout << "newUser insert into user success!" << std::endl;
        return newId;
    }
    catch (const sql::SQLException& e)
    {
        try
        {
            con->_con->rollback();
        }
        catch (...)
        {
        }
        LogSqlException(e);
        return -1;
    }
}

DbStatus MysqlDao::CheckEmail(const std::string& name, const std::string& email)
{
    auto con = pool_->getConnection();
    Defer defer([this, &con]() {
        pool_->returnConnection(std::move(con));
    });
    if (con == nullptr || con->_con == nullptr)
    {
        return DbStatus::Error;
    }

    try
    {
        std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("SELECT email FROM user WHERE name = ?"));
        pstmt->setString(1, name);
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        if (!res->next())
        {
            return DbStatus::Mismatch;
        }
        return email == res->getString("email") ? DbStatus::Ok : DbStatus::Mismatch;
    }
    catch (sql::SQLException& e)
    {
        LogSqlException(e);
        return DbStatus::Error;
    }
}

DbStatus MysqlDao::UpdatePwd(const std::string& name, const std::string& newpwd)
{
    auto con = pool_->getConnection();
    Defer defer([this, &con]() {
        pool_->returnConnection(std::move(con));
    });
    if (con == nullptr || con->_con == nullptr)
    {
        return DbStatus::Error;
    }

    try
    {
        std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("UPDATE user SET pwd = ? WHERE name = ?"));
        pstmt->setString(1, newpwd);
        pstmt->setString(2, name);
        int updateCount = pstmt->executeUpdate();
        std::cout << "Updated rows: " << updateCount << std::endl;
        if (updateCount > 0)
        {
            return DbStatus::Ok;
        }
        // MySQL reports changed rows, so re-setting the same password also yields 0
        std::unique_ptr<sql::PreparedStatement> pexist(con->_con->prepareStatement("SELECT 1 FROM user WHERE name = ?"));
        pexist->setString(1, name);
        std::unique_ptr<sql::ResultSet> res(pexist->executeQuery());
        return res->next() ? DbStatus::Ok : DbStatus::Mismatch;
    }
    catch (sql::SQLException& e)
    {
        LogSqlException(e);
        return DbStatus::Error;
    }
}

DbStatus MysqlDao::CheckPwd(const std::string& email, const std::string& pwd, UserInfo& userInfo)
{
    auto con = pool_->getConnection();
    Defer defer([this, &con]() {
        pool_->returnConnection(std::move(con));
    });
    if (con == nullptr || con->_con == nullptr)
    {
        std::cerr << "Failed to get connection from pool." << std::endl;
        return DbStatus::Error;
    }

    try
    {
        std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("SELECT pwd, email, uid FROM user WHERE email = ?"));
        pstmt->setString(1, email);
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        if (!res->next())
        {
            std::cerr << "No user found with the given email." << std::endl;
            return DbStatus::Mismatch;
        }

        std::string origin_pwd = res->getString("pwd");
        if (pwd != origin_pwd)
        {
            std::cerr << "Password does not match." << std::endl;
            return DbStatus::Mismatch;
        }

        userInfo.email = res->getString("email");
        userInfo.uid = res->getInt("uid");
        userInfo.pwd = origin_pwd;
        return DbStatus::Ok;
    }
    catch (sql::SQLException& e)
    {
        LogSqlException(e);
        return DbStatus::Error;
    }
}
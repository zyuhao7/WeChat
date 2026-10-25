#pragma once
#include "const.h"
#include <thread>
#include <jdbc/mysql_driver.h>
#include <jdbc/mysql_connection.h>
#include <jdbc/cppconn/prepared_statement.h>
#include <jdbc/cppconn/resultset.h>
#include <jdbc/cppconn/statement.h>
#include <jdbc/cppconn/exception.h>
#include "data.h"
#include <memory>
#include <queue>
#include <mutex>
#include <utility>

class SqlConnection
{
public:
    SqlConnection(sql::Connection* con, int64_t lasttime)
        : _con(con), _last_oper_time(lasttime) {}
    std::unique_ptr<sql::Connection> _con;
    int64_t _last_oper_time;
};

class MySqlPool
{
public:
    MySqlPool(const std::string& url, const std::string& user, const std::string& pass, const std::string& schema, int poolSize)
        : url_(url), user_(user), pass_(pass), schema_(schema), poolSize_(poolSize), b_stop_(false)
    {
        try
        {
            for (int i = 0; i < poolSize_; ++i)
            {
                sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();
                auto* con = driver->connect(url_, user_, pass_);
                con->setSchema(schema_);
                // get the current timestamp
                auto currentTime = std::chrono::system_clock::now().time_since_epoch();
                // convert the timestamp to seconds
                long long timestamp = std::chrono::duration_cast<std::chrono::seconds>(currentTime).count();
                pool_.push(std::make_unique<SqlConnection>(con, timestamp));
            }

            _check_thread = std::thread([this]() {
                while (!b_stop_)
                {
                    checkConnection();
                    std::unique_lock<std::mutex> lock(check_mutex_);
                    check_cond_.wait_for(lock, std::chrono::seconds(60), [this] { return b_stop_.load(); });
                }
            });
        }
        catch (sql::SQLException& e)
        {
            // handle the exception
            std::cout << "mysql pool init failed, error is " << e.what() << std::endl;
        }
    }

    void checkConnection()
    {
        std::lock_guard<std::mutex> guard(mutex_);
        int poolsize = pool_.size();
        // get the current timestamp
        auto currentTime = std::chrono::system_clock::now().time_since_epoch();
        // convert the timestamp to seconds
        long long timestamp = std::chrono::duration_cast<std::chrono::seconds>(currentTime).count();
        for (int i = 0; i < poolsize; i++)
        {
            auto con = std::move(pool_.front());
            pool_.pop();
            Defer defer([this, &con]() {
                pool_.push(std::move(con));
            });

            if (timestamp - con->_last_oper_time < 5)
            {
                continue;
            }

            try
            {
                std::unique_ptr<sql::Statement> stmt(con->_con->createStatement());
                stmt->executeQuery("SELECT 1");
                con->_last_oper_time = timestamp;
                // std::cout << "execute timer alive query , cur is " << timestamp << std::endl;
            }
            catch (sql::SQLException& e)
            {
                std::cout << "Error keeping connection alive: " << e.what() << std::endl;
                // recreate the connection and replace the old one; an escaping throw would kill the process
                try
                {
                    sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();
                    std::unique_ptr<sql::Connection> newcon(driver->connect(url_, user_, pass_));
                    newcon->setSchema(schema_);
                    con->_con = std::move(newcon);
                    con->_last_oper_time = timestamp;
                }
                catch (sql::SQLException& e2)
                {
                    std::cout << "mysql reconnect failed, retry next round: " << e2.what() << std::endl;
                }
            }
        }
    }

    std::unique_ptr<SqlConnection> getConnection()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        cond_.wait(lock, [this] {
			if (b_stop_) {
				return true;
			}
			return !pool_.empty(); });
        if (b_stop_)
        {
            return nullptr;
        }
        std::unique_ptr<SqlConnection> con(std::move(pool_.front()));
        pool_.pop();
        return con;
    }

    void returnConnection(std::unique_ptr<SqlConnection> con)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        if (b_stop_)
        {
            return;
        }
        pool_.push(std::move(con));
        cond_.notify_one();
    }

    void Close()
    {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            std::lock_guard<std::mutex> check_lock(check_mutex_);
            b_stop_ = true;
        }
        cond_.notify_all();
        check_cond_.notify_all();
        if (_check_thread.joinable() && _check_thread.get_id() != std::this_thread::get_id())
        {
            _check_thread.join();
        }
    }

    ~MySqlPool()
    {
        Close();
        std::unique_lock<std::mutex> lock(mutex_);
        while (!pool_.empty())
        {
            pool_.pop();
        }
    }

private:
    std::string url_;
    std::string user_;
    std::string pass_;
    std::string schema_;
    int poolSize_;
    std::queue<std::unique_ptr<SqlConnection>> pool_;
    std::mutex mutex_;
    std::condition_variable cond_;
    std::atomic<bool> b_stop_;
    std::thread _check_thread;
    std::mutex check_mutex_;
    std::condition_variable check_cond_;
};

class MysqlDao
{
public:
    MysqlDao();
    ~MysqlDao();
    int RegUser(const std::string& name, const std::string& email, const std::string& pwd);
    bool CheckEmail(const std::string& name, const std::string& email);
    bool UpdatePwd(const std::string& name, const std::string& pwd);
    bool CheckPwd(const std::string& name, const std::string& pwd, UserInfo& userInfo);
    bool AddFriendApply(const int& from, const int& to);
    bool AuthFriendApply(const int& from, const int& to);
    bool AddFriend(const int& from, const int& to, std::string back_name);
    std::shared_ptr<UserInfo> GetUser(int uid);
    std::shared_ptr<UserInfo> GetUser(std::string name);
    bool GetApplyList(int touid, std::vector<std::shared_ptr<ApplyInfo>>& applyList, int offset, int limit);
    bool GetFriendList(int self_id, std::vector<std::shared_ptr<UserInfo>>& user_info);
    bool AddChatMessages(int from_uid, int to_uid,
                         const std::vector<std::pair<std::string, std::string>>& messages);
    bool GetChatHistory(int self_uid, int peer_uid, long long last_id, int limit,
                        std::vector<std::shared_ptr<ChatMessageInfo>>& messages);

private:
    std::unique_ptr<MySqlPool> pool_;
};

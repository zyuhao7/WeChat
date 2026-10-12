#pragma once
#include "const.h"
#include <queue>
#include <thread>
#include <atomic>
#include <condition_variable>

// _con may be null when the database was unreachable; the pool keeps the slot and
// reconnects on the next borrow or keep-alive round instead of shrinking for good.
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
        for (int i = 0; i < poolSize_; ++i)
        {
            pool_.push(std::make_unique<SqlConnection>(connect(), nowSeconds()));
        }

        _check_thread = std::thread([this]() {
            std::unique_lock<std::mutex> lock(mutex_);
            while (!stop_cond_.wait_for(lock, std::chrono::seconds(60), [this] { return b_stop_.load(); }))
            {
                checkConnection();
            }
        });
    }

    std::unique_ptr<SqlConnection> getConnection()
    {
        std::unique_ptr<SqlConnection> con;
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
            con = std::move(pool_.front());
            pool_.pop();
        }
        if (!con->_con)
        {
            con->_con.reset(connect());
            con->_last_oper_time = nowSeconds();
        }
        return con;
    }

    void returnConnection(std::unique_ptr<SqlConnection> con)
    {
        if (!con)
        {
            return;
        }
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
            b_stop_ = true;
        }
        cond_.notify_all();
        stop_cond_.notify_all();
    }

    ~MySqlPool()
    {
        Close();
        if (_check_thread.joinable())
        {
            _check_thread.join();
        }
        std::unique_lock<std::mutex> lock(mutex_);
        while (!pool_.empty())
        {
            pool_.pop();
        }
    }

private:
    static int64_t nowSeconds()
    {
        auto currentTime = std::chrono::system_clock::now().time_since_epoch();
        return std::chrono::duration_cast<std::chrono::seconds>(currentTime).count();
    }

    // Returns nullptr instead of throwing so a down database never kills the caller's thread.
    sql::Connection* connect()
    {
        try
        {
            sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();
            std::unique_ptr<sql::Connection> con(driver->connect(url_, user_, pass_));
            con->setSchema(schema_);
            return con.release();
        }
        catch (sql::SQLException& e)
        {
            std::cout << "mysql connect failed, error is " << e.what() << std::endl;
            return nullptr;
        }
    }

    // Called with mutex_ held.
    void checkConnection()
    {
        int poolsize = pool_.size();
        long long timestamp = nowSeconds();
        for (int i = 0; i < poolsize; i++)
        {
            auto con = std::move(pool_.front());
            pool_.pop();
            Defer defer([this, &con]() {
                pool_.push(std::move(con));
            });

            if (con->_con && timestamp - con->_last_oper_time < 5)
            {
                continue;
            }

            if (con->_con)
            {
                try
                {
                    std::unique_ptr<sql::Statement> stmt(con->_con->createStatement());
                    std::unique_ptr<sql::ResultSet> res(stmt->executeQuery("SELECT 1"));
                    con->_last_oper_time = timestamp;
                    continue;
                }
                catch (sql::SQLException& e)
                {
                    std::cout << "Error keeping connection alive: " << e.what() << std::endl;
                }
            }
            con->_con.reset(connect());
            con->_last_oper_time = timestamp;
        }
    }

    std::string url_;
    std::string user_;
    std::string pass_;
    std::string schema_;
    int poolSize_;
    std::queue<std::unique_ptr<SqlConnection>> pool_;
    std::mutex mutex_;
    std::condition_variable cond_;
    std::condition_variable stop_cond_;
    std::atomic<bool> b_stop_;
    std::thread _check_thread;
};

struct UserInfo
{
    std::string name;
    std::string pwd;
    int uid;
    std::string email;
};

enum class DbStatus
{
    Ok,
    Mismatch, // query succeeded but the row was absent or did not match
    Error,    // no connection or the query threw
};

class MysqlDao
{
public:
    MysqlDao();
    ~MysqlDao();
    // >0 new uid, 0 email or name already taken, -1 database error
    int RegUserTransaction(const std::string& name, const std::string& email, const std::string& pwd, const std::string& icon);
    DbStatus CheckEmail(const std::string& name, const std::string& email);
    DbStatus UpdatePwd(const std::string& name, const std::string& pwd);
    DbStatus CheckPwd(const std::string& email, const std::string& pwd, UserInfo& userInfo);

private:
    std::unique_ptr<MySqlPool> pool_;
};

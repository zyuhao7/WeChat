#pragma once
#include "hiredis.h"
#include <queue>
#include <atomic>
#include <mutex>
#include <iostream>
#include <condition_variable>
#include <thread>
#include <chrono>
#include "Singleton.h"
#include <string>

class RedisConPool {
public:
    RedisConPool(size_t poolSize, const std::string& host, int port, const std::string& pwd)
        : b_stop_(false), poolSize_(poolSize), host_(host), pwd_(pwd), port_(port), fail_count_(0), counter_(0) {
        for (size_t i = 0; i < poolSize_; ++i) {
            auto* context = connectAndAuth();
            if (context == nullptr) {
                // let the health check retry it instead of running permanently short
                fail_count_++;
                continue;
            }
            connections_.push(context);
        }

        check_thread_ = std::thread([this]() {
            while (!b_stop_) {
                counter_++;
                if (counter_ >= 60) {
                    checkThreadPro();
                    counter_ = 0;
                }
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
            });
    }

    ~RedisConPool() {
        Close();
        ClearConnections();
    }

    void ClearConnections()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        while (!connections_.empty())
        {
            auto* context = connections_.front();
            redisFree(context);
            connections_.pop();
        }
    }

    redisContext* getConnection() {
        std::unique_lock<std::mutex> lock(mutex_);
        cond_.wait(lock, [this] {
            if (b_stop_) {
                return true;
            }
            return !connections_.empty();
            });
        //if stopped, return a null pointer
        if (b_stop_) {
            return  nullptr;
        }
        auto* context = connections_.front();
        connections_.pop();
        return context;
    }

    // non-blocking context acquisition: nullptr when stopping or nothing is idle
    redisContext* getConnNonBlock()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        if (b_stop_ || connections_.empty()) return nullptr;

        auto* context = connections_.front();
        connections_.pop();
        return context;
    }

    // recycle the redisContext connection for reuse.
    void returnConnection(redisContext* context) {
        if (context == nullptr) {
            return;
        }
        {
            std::lock_guard<std::mutex> lock(mutex_);
            // a context with err set is dead for good; hand its slot to the reconnect loop
            if (!b_stop_ && context->err == 0) {
                connections_.push(context);
                cond_.notify_one();
                return;
            }
        }
        redisFree(context);
        if (!b_stop_) {
            fail_count_++;
        }
    }

    void Close() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            b_stop_ = true;
        }
        // wake the condition variable waiting for connections, signaling shutdown.
        cond_.notify_all();
        if (check_thread_.joinable()) {
            check_thread_.join();
        }
    }

    size_t IdleCount() {
        std::lock_guard<std::mutex> lock(mutex_);
        return connections_.size();
    }

    int FailCount() const {
        return fail_count_;
    }

private:
    redisContext* connectAndAuth()
    {
        auto* context = redisConnect(host_.c_str(), port_);
        if (context == nullptr || context->err != 0) {
            if (context != nullptr) {
                redisFree(context);
            }
            return nullptr;
        }
        // an empty password means the server has no requirepass; AUTH "" would just fail
        if (pwd_.empty()) {
            return context;
        }

        auto* reply = (redisReply*)redisCommand(context, "AUTH %s", pwd_.c_str());
        if (reply == nullptr || reply->type == REDIS_REPLY_ERROR) {
            std::cout << "redis AUTH failed" << std::endl;
            if (reply != nullptr) {
                freeReplyObject(reply);
            }
            redisFree(context);
            return nullptr;
        }
        freeReplyObject(reply);
        return context;
    }

    bool reconnect()
    {
        auto* context = connectAndAuth();
        if (context == nullptr) {
            return false;
        }
        std::lock_guard<std::mutex> lock(mutex_);
        if (b_stop_) {
            redisFree(context);
            return true;
        }
        connections_.push(context);
        cond_.notify_one();
        return true;
    }

    void checkThreadPro()
    {
        size_t pool_size = IdleCount();

        for (size_t i = 0; i < pool_size && !b_stop_; ++i) {
            auto* context = getConnNonBlock();
            if (context == nullptr)
            {
                break;
            }

            auto* reply = (redisReply*)redisCommand(context, "PING");
            bool alive = reply != nullptr && context->err == 0 && reply->type != REDIS_REPLY_ERROR;
            if (reply != nullptr) {
                freeReplyObject(reply);
            }
            if (!alive) {
                std::cout << "redis ping failed, dropping connection" << std::endl;
                redisFree(context);
                fail_count_++;
                continue;
            }
            returnConnection(context);
        }

        while (fail_count_ > 0 && !b_stop_)
        {
            if (!reconnect())
                break; // leave it for the next retry.
            fail_count_--;
        }
    }

    std::atomic<bool> b_stop_;
    size_t poolSize_;
    std::string host_;
    std::string pwd_;
    int port_;
    std::queue<redisContext*> connections_;
    std::atomic<int> fail_count_;
    std::mutex mutex_;
    std::condition_variable cond_;
    std::thread check_thread_;
    int counter_;
};

class RedisMgr : public Singleton<RedisMgr>, public std::enable_shared_from_this<RedisMgr>
{
    friend class Singleton<RedisMgr>;
public:
    ~RedisMgr();
    bool Get(const std::string& key, std::string& value);
    bool Set(const std::string& key, const std::string& value);
    bool LPush(const std::string& key, const std::string& value);
    bool LPop(const std::string& key, std::string& value);
    bool RPush(const std::string& key, const std::string& value);
    bool RPop(const std::string& key, std::string& value);
    bool HSet(const std::string& key, const std::string& hkey, const std::string& value);
    bool HSet(const char* key, const char* hkey, const char* hvalue, size_t hvaluelen);
    std::string HGet(const std::string& key, const std::string& hkey);
    bool HDel(const std::string& key, const std::string& field);
    bool Del(const std::string& key);
    bool ExistsKey(const std::string& key);
    void Close();

    std::string acquireLock(const std::string& lockName, int lockTimeout, int acquireTimeout);
    bool releaseLock(const std::string& lockName, const std::string& identifier);

    void IncreaseCount(std::string server_name);
    void DecreaseCount(std::string server_name);
    void InitCount(std::string server_name);
    void DelCount(std::string server_name);

private:
    RedisMgr();
    std::unique_ptr<RedisConPool> _con_pool;
};


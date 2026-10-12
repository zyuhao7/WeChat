#pragma once
#include "const.h"
#include "hiredis.h"
#include <queue>
#include <atomic>
#include <mutex>
#include <iostream>
#include <condition_variable>
#include <thread>
#include <chrono>
#include <string>
#include "Singleton.h"

class RedisConPool {
public:
    RedisConPool(size_t poolSize, const std::string& host, int port, const std::string& pwd)
        : b_stop_(false), poolSize_(poolSize), host_(host), pwd_(pwd), port_(port), fail_count_(0) {
        for (size_t i = 0; i < poolSize_; ++i) {
            auto* context = connectOne();
            if (context == nullptr) {
                // the check thread keeps retrying until the pool is full
                fail_count_++;
                continue;
            }
            connections_.push(context);
        }

        check_thread_ = std::thread([this]() { checkLoop(); });
    }

    ~RedisConPool() {
        Close();
    }

    // returns nullptr if the pool is closed or no connection frees up in time
    redisContext* getConnection() {
        std::unique_lock<std::mutex> lock(mutex_);
        bool ready = cond_.wait_for(lock, std::chrono::seconds(kWaitConnSeconds), [this] {
            return b_stop_ || !connections_.empty();
            });
        if (!ready || b_stop_) {
            return nullptr;
        }
        auto* context = connections_.front();
        connections_.pop();
        return context;
    }

    // non-blocking context acquisition
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
        // hiredis contexts are unusable once err is set; replace it instead of recycling
        if (context->err != 0) {
            redisFree(context);
            fail_count_++;
            return;
        }
        std::lock_guard<std::mutex> lock(mutex_);
        if (b_stop_) {
            redisFree(context);
            return;
        }
        connections_.push(context);
        cond_.notify_one();
    }

    void Close() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (b_stop_) {
                return;
            }
            b_stop_ = true;
        }
        // wake the condition variable waiting for connections, signaling shutdown.
        cond_.notify_all();
        if (check_thread_.joinable()) {
            check_thread_.join();
        }
        clearConnections();
    }

private:
    static constexpr int kConnectTimeoutSeconds = 3;
    static constexpr int kWaitConnSeconds = 5;
    static constexpr int kReconnectIntervalSeconds = 5;
    static constexpr int kPingIntervalSeconds = 60;

    void clearConnections()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        while (!connections_.empty())
        {
            redisFree(connections_.front());
            connections_.pop();
        }
    }

    // connect and authenticate one context; nullptr on any failure
    redisContext* connectOne()
    {
        timeval timeout{ kConnectTimeoutSeconds, 0 };
        auto* context = redisConnectWithTimeout(host_.c_str(), port_, timeout);
        if (context == nullptr || context->err != 0)
        {
            if (context != nullptr) {
                std::cout << "redis connect failed: " << context->errstr << std::endl;
                redisFree(context);
            }
            return nullptr;
        }
        redisSetTimeout(context, timeout);

        if (!pwd_.empty()) {
            auto* reply = (redisReply*)redisCommand(context, "AUTH %s", pwd_.c_str());
            bool ok = reply != nullptr && reply->type != REDIS_REPLY_ERROR;
            if (reply != nullptr) {
                freeReplyObject(reply);
            }
            if (!ok) {
                std::cout << "AUTH failed" << std::endl;
                redisFree(context);
                return nullptr;
            }
        }
        return context;
    }

    void checkLoop()
    {
        int ticks = 0;
        while (!b_stop_) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            ++ticks;
            if (ticks % kPingIntervalSeconds == 0) {
                pingIdle();
                ticks = 0;
            }
            if (ticks % kReconnectIntervalSeconds == 0) {
                refill();
            }
        }
    }

    // PING every idle connection once, dropping the dead ones
    void pingIdle()
    {
        size_t pool_size;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            pool_size = connections_.size();
        }

        for (size_t i = 0; i < pool_size && !b_stop_; ++i) {
            auto* context = getConnNonBlock();
            if (context == nullptr)
            {
                break;
            }

            auto* reply = (redisReply*)redisCommand(context, "PING");
            bool ok = reply != nullptr && context->err == 0 && reply->type != REDIS_REPLY_ERROR;
            if (reply != nullptr) {
                freeReplyObject(reply);
            }
            if (!ok) {
                std::cout << "redis ping failed" << std::endl;
                redisFree(context);
                fail_count_++;
                continue;
            }
            returnConnection(context);
        }
    }

    // replace connections lost at startup or dropped later
    void refill()
    {
        while (fail_count_ > 0 && !b_stop_)
        {
            auto* context = connectOne();
            if (context == nullptr) {
                break; // leave it for the next retry.
            }
            fail_count_--;
            std::cout << "redis reconnect succeeded" << std::endl;
            returnConnection(context);
        }
    }

    std::atomic<bool> b_stop_;
    size_t poolSize_;
    std::string host_;
    std::string pwd_;
    int port_;
    std::atomic<int> fail_count_;
    std::queue<redisContext*> connections_;
    std::mutex mutex_;
    std::condition_variable cond_;
    std::thread check_thread_;
};

class RedisMgr : public Singleton<RedisMgr>, public std::enable_shared_from_this<RedisMgr>
{
    friend class Singleton<RedisMgr>;
public:
    ~RedisMgr();
    bool Get(const std::string& key, std::string& value);
    bool Set(const std::string& key, const std::string& value);
    bool SetEx(const std::string& key, const std::string& value, int ttl_seconds);
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


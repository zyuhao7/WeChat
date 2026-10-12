#pragma once
#include "const.h"
#include "hiredis.h"
#include <queue>
#include <atomic>
#include <mutex>
#include <thread>
#include <chrono>
#include <iostream>
#include <condition_variable>
#include <stdexcept>
#include <string>
#include "Singleton.h"

// Every slot holds a context, possibly a broken one; broken contexts are reconnected on
// borrow and by the PING thread, so a Redis outage never shrinks the pool to nothing.
class RedisConPool
{
public:
    RedisConPool(size_t poolSize, const std::string& host, int port, const std::string& pwd)
        : b_stop_(false), poolSize_(poolSize), host_(host), pwd_(pwd), port_(port), counter_(0)
    {
        for (size_t i = 0; i < poolSize_; ++i)
        {
            auto* context = connect();
            if (context != nullptr)
            {
                connections_.push(context);
            }
        }
        // fail fast: with no usable connection every request would block in getConnection()
        if (connections_.empty())
        {
            throw std::runtime_error("cannot connect to redis at " + host_ + ":" + std::to_string(port_));
        }

        check_thread_ = std::thread([this]() {
            while (!b_stop_)
            {
                counter_++;
                if (counter_ >= 60)
                {
                    checkThread();
                    counter_ = 0;
                }

                std::this_thread::sleep_for(std::chrono::seconds(1)); // send a PING every 60 seconds
            }
        });
    }

    ~RedisConPool()
    {
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

    redisContext* getConnection()
    {
        redisContext* context = nullptr;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            cond_.wait(lock, [this] {
                if (b_stop_)
                {
                    return true;
                }
                return !connections_.empty();
            });
            // if stopped, return a null pointer
            if (b_stop_)
            {
                return nullptr;
            }
            context = connections_.front();
            connections_.pop();
        }
        if (context->err != 0)
        {
            auto* fresh = connect();
            if (fresh != nullptr)
            {
                redisFree(context);
                context = fresh;
            }
        }
        return context;
    }

    void returnConnection(redisContext* context)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (b_stop_)
        {
            redisFree(context);
            return;
        }
        connections_.push(context);
        cond_.notify_one();
    }

    void Close()
    {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            b_stop_ = true;
        }
        cond_.notify_all();
        if (check_thread_.joinable())
        {
            check_thread_.join();
        }
    }

private:
    // Returns an authenticated context or nullptr; never leaks a context or reply.
    redisContext* connect()
    {
        struct timeval timeout = {2, 0};
        auto* context = redisConnectWithTimeout(host_.c_str(), port_, timeout);
        if (context == nullptr || context->err != 0)
        {
            std::cout << "redis connect failed: " << (context ? context->errstr : "out of memory") << std::endl;
            if (context != nullptr)
            {
                redisFree(context);
            }
            return nullptr;
        }
        redisSetTimeout(context, timeout);

        if (pwd_.empty())
        {
            return context;
        }
        auto reply = (redisReply*)redisCommand(context, "AUTH %s", pwd_.c_str());
        if (reply == nullptr || reply->type == REDIS_REPLY_ERROR)
        {
            std::cout << "AUTH failed" << std::endl;
            if (reply != nullptr)
            {
                freeReplyObject(reply);
            }
            redisFree(context);
            return nullptr;
        }
        freeReplyObject(reply);
        return context;
    }

    void checkThread()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (b_stop_)
        {
            return;
        }
        auto pool_size = connections_.size();
        for (size_t i = 0; i < pool_size && !b_stop_; i++)
        {
            auto* context = connections_.front();
            connections_.pop();
            auto reply = (redisReply*)redisCommand(context, "PING");
            bool alive = reply != nullptr && context->err == 0;
            if (reply != nullptr)
            {
                freeReplyObject(reply);
            }
            if (!alive)
            {
                std::cout << "redis ping failed, reconnecting" << std::endl;
                auto* fresh = connect();
                if (fresh != nullptr)
                {
                    redisFree(context);
                    context = fresh;
                }
            }
            // a still-broken context keeps its slot and is retried next round
            connections_.push(context);
        }
    }
    std::atomic<bool> b_stop_;
    size_t poolSize_;
    std::string host_;
    std::string pwd_;
    int port_;
    std::queue<redisContext*> connections_;
    std::mutex mutex_;
    std::condition_variable cond_;
    std::thread check_thread_;
    int counter_;
};

class RedisMgr : public Singleton<RedisMgr>,
                 public std::enable_shared_from_this<RedisMgr>
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
    void Close()
    {
        _con_pool->Close();
        _con_pool->ClearConnections();
    }

private:
    RedisMgr();
    unique_ptr<RedisConPool> _con_pool;
};

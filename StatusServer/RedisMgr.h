#pragma once
#include "const.h"
#include "hiredis.h"
#include <queue>
#include <atomic>
#include <mutex>
#include "Singleton.h"

class RedisConPool {
public:
    RedisConPool(size_t poolSize, const char* host, int port, const char* pwd)
        : poolSize_(poolSize), host_(host), pwd_(pwd), port_(port),b_stop_(false), counter_(0), fail_count_(0) {
        for (size_t i = 0; i < poolSize_; ++i) {
            auto* context = redisConnect(host, port);
            if (context == nullptr || context->err != 0) {
                if (context != nullptr) {
                    redisFree(context);
                }
                continue;
            }

            auto reply = (redisReply*)redisCommand(context, "AUTH %s", pwd);
            if (reply->type == REDIS_REPLY_ERROR) {
                std::cout << "��֤ʧ��" << std::endl;
                //ִ�гɹ� �ͷ�redisCommandִ�к󷵻ص�redisReply��ռ�õ��ڴ�
                freeReplyObject(reply);
                continue;
            }

            //ִ�гɹ� �ͷ�redisCommandִ�к󷵻ص�redisReply��ռ�õ��ڴ�
            freeReplyObject(reply);
            std::cout << "��֤�ɹ�" << std::endl;
            connections_.push(context);
        }

        check_thread_ = std::thread([this]() {
            while (!b_stop_) {
                counter_++;
                if (counter_ >= 60) {
                    checkThreadPro(); 
                    counter_ = 0;
                }
                std::this_thread::sleep_for(std::chrono::seconds(1)); // ÿ�� 30 �뷢��һ�� PING ����
            }
            });


    }

    ~RedisConPool() {}

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
        //���ֹͣ��ֱ�ӷ��ؿ�ָ��
        if (b_stop_) {
            return  nullptr;
        }
        auto* context = connections_.front();
        connections_.pop();
        return context;
    }

    // ��������ȡ context
    redisContext* getConnNonBlock()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        // ���ͣ������û������,���ȴ�,ֱ�ӷ��� nullptr
        if (b_stop_ || !connections_.empty()) return nullptr;

        // auto �� auto*  ������, ��ϰ��.
        auto* context = connections_.front();
        connections_.pop();
        return context;
    }

    // ���� rediscontext �����Ա�����.
    void returnConnection(redisContext* context) {
        std::lock_guard<std::mutex> lock(mutex_);
        // ���ͣ��, ������, ֱ�ӷ���.
        if (b_stop_) {
            return;
        }
        connections_.push(context);
        cond_.notify_one();
    }

    void Close() {
        b_stop_ = true;
        // ���ѵȴ���ȡ���ӵ���������, ֪ͨ�رշ�����.
        cond_.notify_all();
        check_thread_.join();
    }

private:
    // ��������
    bool reconnect()
    {
        auto context = redisConnect(host_, port_);
        if (context == nullptr || context->err != 0)
        {
            if (context != nullptr)
                redisFree(context);
            // ����ʧ��
            return false;
        }

        auto reply = (redisReply*)redisCommand(context, "AUTH %s", pwd_);
        if (reply->type == REDIS_REPLY_ERROR)
        {
            std::cout << "������֤ʧ��!" << std::endl;
            // ִ��ʧ��, �ͷ� redisCommand ִ�к󷵻ص� redisReply��ռ�õ��ڴ�
            freeReplyObject(reply);
            redisFree(context);
            return false;
        }
        // ִ�гɹ�, �ͷ� redisCommand ִ�к󷵻ص� redisReply��ռ�õ��ڴ�
        freeReplyObject(reply);
        std::cout << "������֤�ɹ�!" << std::endl;
        // �������պ�������context.
        returnConnection(context);
        return true;
    }

    void checkThreadPro()
    {
        size_t pool_size;
        {
            // ���õ���ǰ������.
            std::lock_guard<std::mutex> lock(mutex_);
            pool_size = connections_.size();
        }

        for (int i = 0; i < pool_size && !b_stop_; ++i) {
            redisContext* ctx = nullptr;
            // 1) ȡ��һ������(������)
            bool bsuccess = false;
            auto* context = getConnNonBlock();
            if (context == nullptr)
            {
                break;
            }

            redisReply* reply = nullptr;
            try
            {
                reply = (redisReply*)redisCommand(context, "PING"); // ���� PING ����.
                // 2) �ȿ��ײ�I/O Э�����û�д�
                if (context->err)
                {
                    std::cout << "Coonnection error: " << context->err << std::endl;
                    if (reply) 
                        freeReplyObject(reply);

                    redisFree(context);
                    fail_count_++;
                    continue;
                }
                // 3) �ٿ� Reply �����ص��ǲ��� ERROR
                if (!reply || reply->type == REDIS_REPLY_ERROR)
                {
                    std::cout << "reply is null, redis ping failed: " << std::endl;
                    if (reply)
                        freeReplyObject(reply);
                    redisFree(context);
                    fail_count_++;
                    continue;
                }
                // 4) �����û����, �򻹻�ȥ
                std::cout << "connection alive... " << std::endl;
                freeReplyObject(reply);
                returnConnection(context);
            }
            catch (const std::exception& e)
            {
                if (reply)
                    freeReplyObject(reply);
                redisFree(context);
                fail_count_++;

            }
        }
        // ִ����������
        while (fail_count_ > 0)
        {
            auto res = reconnect();
            if (res) fail_count_--;
            else
                break; // �����´�������.
        }
    }

    void checkThread()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (b_stop_) return;

        auto pool_size = connections_.size();
        for (int i = 0; i < pool_size && !b_stop_; ++i)
        {
            auto* context = connections_.front();
            connections_.pop();
            try
            {
                auto reply = (redisReply*)redisCommand(context, "PING");
                if (!reply)
                {
                    std::cout << " reply is null, redis ping failed! " << std::endl;
                    connections_.push(context);
                    continue;
                }
                freeReplyObject(reply);
                connections_.push(context);
            }
            catch (const std::exception& e)
            {
                std::cout << "Error keeping connection alive : " << e.what() << std::endl;
                redisFree(context);
                context = redisConnect(host_, port_);
                if (context == nullptr || context->err != 0)
                {
                    if (context != nullptr)
                        redisFree(context);
                    continue;
                }
                auto reply = (redisReply*)redisCommand(context, "AUTH %s", pwd_);
                if (reply->type == REDIS_REPLY_ERROR)
                {
                    std::cout << "��֤ʧ��! " << std::endl;
                    freeReplyObject(reply);
                    continue;
                }
                freeReplyObject(reply);
                std::cout << "��֤�ɹ�! " << std::endl;
                connections_.push(context);
            }
        }
    }

    std::atomic<bool> b_stop_;
    size_t poolSize_;
    const char* host_;
    const char* pwd_;
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


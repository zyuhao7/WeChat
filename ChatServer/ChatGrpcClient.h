#pragma once
#pragma once
#include "Singleton.h"
#include <grpcpp/grpcpp.h>
#include "message.grpc.pb.h"
#include "message.pb.h"
#include <queue>
#include <condition_variable>
#include <unordered_map>
#include "data.h"
#include <json/json.h>
#include <json/value.h>
#include <json/reader.h>

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

using message::AddFriendReq;
using message::AddFriendRsp;

using message::AuthFriendReq;
using message::AuthFriendRsp;

using message::ChatService;
using message::GetChatServerRsp;
using message::LoginReq;
using message::LoginRsp;

using message::TextChatData;
using message::TextChatMsgReq;
using message::TextChatMsgRsp;

using message::FileMsgReq;
using message::FileMsgRsp;

using message::KickUserReq;
using message::KickUserRsp;

class ChatConPool
{
public:
    ChatConPool(size_t poolSize, std::string host, std::string port)
        : poolSize_(poolSize),
          host_(host),
          port_(port),
          b_stop_(false)
    {
        for (int i = 0; i < poolSize_; ++i)
        {
            std::shared_ptr<Channel> channel = grpc::CreateChannel(host + ":" + port,
                                                                   grpc::InsecureChannelCredentials());
            connections_.push(ChatService::NewStub(channel));
        }
    }

    ~ChatConPool()
    {
        Close();
        std::lock_guard<std::mutex> lock(mutex_);
        while (!connections_.empty())
        {
            connections_.pop();
        }
    }

    std::unique_ptr<ChatService::Stub> getConnection()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        cond_.wait(lock, [this]() {
            if (b_stop_)
                return true;
            return !connections_.empty();
        });
        if (b_stop_)
            return nullptr;

        auto context = std::move(connections_.front());
        connections_.pop();
        return context;
    }

    void returnConnection(std::unique_ptr<ChatService::Stub> context)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        if (b_stop_)
            return;
        connections_.push(std::move(context));
        cond_.notify_one();
    }

    void Close()
    {
        {
            // set under the lock so a waiter cannot miss the wakeup between its check and wait
            std::lock_guard<std::mutex> lock(mutex_);
            b_stop_ = true;
        }
        cond_.notify_all();
    }

private:
    std::atomic<bool> b_stop_;
    size_t poolSize_;
    std::string host_;
    std::string port_;
    std::queue<std::unique_ptr<ChatService::Stub>> connections_;
    std::mutex mutex_;
    std::condition_variable cond_;
};

class ChatGrpcClient : public Singleton<ChatGrpcClient>
{
    friend class Singleton<ChatGrpcClient>;

public:
    ~ChatGrpcClient() {}
    AddFriendRsp NotifyAddFriend(std::string server_ip, const AddFriendReq& req);
    AuthFriendRsp NotifyAuthFriend(std::string server_ip, const AuthFriendReq& req);
    TextChatMsgRsp NotifyTextChatMsg(std::string server_ip, const TextChatMsgReq& req, const Json::Value& rtvalue);
    FileMsgRsp NotifyFileMsg(std::string server_ip, const FileMsgReq& req);
    KickUserRsp NotifyKickUser(std::string server_ip, const KickUserReq& req);

private:
    ChatGrpcClient();
    std::unordered_map<std::string, std::unique_ptr<ChatConPool>> _pools;
};

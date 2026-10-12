#include "ChatGrpcClient.h"
#include <chrono>
#include <iostream>
#include "ConfigMgr.h"
#include "const.h"

namespace {

// A dead peer must not stall the single logic thread forever.
constexpr auto kRpcDeadline = std::chrono::seconds(5);

void SetDeadline(ClientContext& context)
{
	context.set_deadline(std::chrono::system_clock::now() + kRpcDeadline);
}

}

ChatGrpcClient::ChatGrpcClient()
{
	auto& cfg = ConfigMgr::Inst();
	auto server_list = cfg["PeerServer"]["Servers"];

	std::vector<std::string> words;
	std::stringstream ss(server_list);
	std::string word;

	while (std::getline(ss, word, ','))
	{
		words.push_back(word);
	}

	for (auto& word : words)
	{
		if (cfg[word]["Name"].empty()) continue;
		_pools[cfg[word]["Name"]] = std::make_unique<ChatConPool>(5, cfg[word]["Host"], cfg[word]["Port"]);
	}
}

// Every reply starts out as RPCFailed and only a completed call may overwrite it, so an
// unknown node, a closed pool or a transport error can never be reported as delivered.
template <typename Req, typename Rsp, typename Call>
static bool CallPeer(std::unordered_map<std::string, std::unique_ptr<ChatConPool>>& pools,
	const std::string& server_name, const Req& req, Rsp& rsp, Call call)
{
	auto it = pools.find(server_name);
	if (it == pools.end())
	{
		std::cout << "no grpc channel to server " << server_name << std::endl;
		return false;
	}

	auto& pool = it->second;
	auto stub = pool->getConnection();
	if (!stub)
	{
		return false;
	}

	ClientContext context;
	SetDeadline(context);
	Status status = call(*stub, &context, req, &rsp);
	pool->returnConnection(std::move(stub));
	if (!status.ok())
	{
		std::cout << "grpc call to " << server_name << " failed: " << status.error_message() << std::endl;
		return false;
	}
	return true;
}

AddFriendRsp ChatGrpcClient::NotifyAddFriend(std::string server_ip, const AddFriendReq& req)
{
	AddFriendRsp rsp;
	bool ok = CallPeer(_pools, server_ip, req, rsp,
		[](ChatService::Stub& stub, ClientContext* ctx, const AddFriendReq& r, AddFriendRsp* out) {
			return stub.NotifyAddFriend(ctx, r, out);
		});
	rsp.set_error(ok ? rsp.error() : static_cast<int>(ErrorCodes::RPCFailed));
	rsp.set_applyuid(req.applyuid());
	rsp.set_touid(req.touid());
	return rsp;
}

AuthFriendRsp ChatGrpcClient::NotifyAuthFriend(std::string server_ip, const AuthFriendReq& req)
{
	AuthFriendRsp rsp;
	bool ok = CallPeer(_pools, server_ip, req, rsp,
		[](ChatService::Stub& stub, ClientContext* ctx, const AuthFriendReq& r, AuthFriendRsp* out) {
			return stub.NotifyAuthFriend(ctx, r, out);
		});
	rsp.set_error(ok ? rsp.error() : static_cast<int>(ErrorCodes::RPCFailed));
	rsp.set_fromuid(req.fromuid());
	rsp.set_touid(req.touid());
	return rsp;
}

TextChatMsgRsp ChatGrpcClient::NotifyTextChatMsg(std::string server_ip,
	const TextChatMsgReq& req, const Json::Value& rtvalue) {

	TextChatMsgRsp rsp;
	bool ok = CallPeer(_pools, server_ip, req, rsp,
		[](ChatService::Stub& stub, ClientContext* ctx, const TextChatMsgReq& r, TextChatMsgRsp* out) {
			return stub.NotifyTextChatMsg(ctx, r, out);
		});
	rsp.set_error(ok ? rsp.error() : static_cast<int>(ErrorCodes::RPCFailed));
	rsp.set_fromuid(req.fromuid());
	rsp.set_touid(req.touid());
	rsp.clear_textmsgs();
	for (const auto& text_data : req.textmsgs()) {
		TextChatData* new_msg = rsp.add_textmsgs();
		new_msg->set_msgid(text_data.msgid());
		new_msg->set_msgcontent(text_data.msgcontent());
	}
	return rsp;
}

KickUserRsp ChatGrpcClient::NotifyKickUser(std::string server_ip, const KickUserReq& req)
{
	KickUserRsp rsp;
	bool ok = CallPeer(_pools, server_ip, req, rsp,
		[](ChatService::Stub& stub, ClientContext* ctx, const KickUserReq& r, KickUserRsp* out) {
			return stub.NotifyKickUser(ctx, r, out);
		});
	rsp.set_error(ok ? rsp.error() : static_cast<int>(ErrorCodes::RPCFailed));
	rsp.set_uid(req.uid());
	return rsp;
}

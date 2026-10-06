#include <gtest/gtest.h>

#include <boost/asio.hpp>

#include "CSession.h"
#include "ChatServiceImpl.h"
#include "UserMgr.h"
#include "const.h"

// The kick RPC reaches back into CServer to clear the session. A service that was never
// wired to a server — which is what ChatServer.cpp shipped, since nothing called
// RegisterServer() — dereferenced a null pointer and took the whole chat node down on any
// cross-node kick. It must answer with an error instead.
TEST(ChatServiceKickUser, UnwiredServiceReportsErrorInsteadOfDereferencingNull)
{
    ChatServiceImpl service;

    grpc::ServerContext context;
    message::KickUserReq req;
    req.set_uid(42);
    message::KickUserRsp rsp;

    const grpc::Status status = service.NotifyKickUser(&context, &req, &rsp);

    EXPECT_TRUE(status.ok());
    EXPECT_EQ(rsp.uid(), 42);
    EXPECT_EQ(rsp.error(), ErrorCodes::RPCFailed);
}

namespace {

// A session with no server behind it is enough for the registry: none of the paths below
// dereference the CServer. They also all return before RedisMgr::Del, so the suite stays
// runnable without a live Redis — the branch that does erase (matching session id) is the
// one that talks to Redis, and is deliberately left out.
std::shared_ptr<CSession> MakeSession(boost::asio::io_context& ioc)
{
    return std::make_shared<CSession>(ioc, nullptr);
}

}  // namespace

TEST(ChatUserMgr, LookupAnswersNullForAnUnknownUid)
{
    EXPECT_EQ(UserMgr::GetInstance()->GetSession(9001), nullptr);
}

TEST(ChatUserMgr, StoresAndReturnsTheSessionBoundToAUid)
{
    boost::asio::io_context ioc;
    auto session = MakeSession(ioc);

    UserMgr::GetInstance()->SetUserSession(9002, session);

    EXPECT_EQ(UserMgr::GetInstance()->GetSession(9002), session);
}

// A newer login overwrites the mapping, so the older connection's cleanup no longer matches
// the stored session id and must leave the current mapping in place.
TEST(ChatUserMgr, RemovalFromAStaleSessionKeepsTheCurrentMapping)
{
    boost::asio::io_context ioc;
    auto session = MakeSession(ioc);
    UserMgr::GetInstance()->SetUserSession(9003, session);

    UserMgr::GetInstance()->RmvUserSession(9003, session->GetSessionId() + "-stale");

    EXPECT_EQ(UserMgr::GetInstance()->GetSession(9003), session);
}

TEST(ChatUserMgr, RemovalForAnUnknownUidIsANoOp)
{
    UserMgr::GetInstance()->RmvUserSession(9004, "any-session");

    EXPECT_EQ(UserMgr::GetInstance()->GetSession(9004), nullptr);
}

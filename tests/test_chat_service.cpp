#include <gtest/gtest.h>

#include "ChatServiceImpl.h"
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

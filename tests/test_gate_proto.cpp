#include <gtest/gtest.h>

#include <string>

#include "message.pb.h"

// Guards the wire format the C++ services and the Qt client compile against. Field numbers
// and wire types are the contract; changing them silently breaks every already-deployed peer.
TEST(ProtoWireFormat, GetVerifyReqFieldOneIsALengthDelimitedString)
{
    message::GetVerifyReq req;
    req.set_email("a");

    const std::string bytes = req.SerializeAsString();

    ASSERT_EQ(bytes.size(), 3u);
    EXPECT_EQ(static_cast<unsigned char>(bytes[0]), 0x0A); // field 1, wire type 2
    EXPECT_EQ(static_cast<unsigned char>(bytes[1]), 0x01); // length 1
    EXPECT_EQ(bytes[2], 'a');
}

TEST(ProtoWireFormat, AddFriendReqApplyUidIsFieldOneVarint)
{
    message::AddFriendReq req;
    req.set_applyuid(1);

    const std::string bytes = req.SerializeAsString();

    ASSERT_EQ(bytes.size(), 2u);
    EXPECT_EQ(static_cast<unsigned char>(bytes[0]), 0x08); // field 1, wire type 0
    EXPECT_EQ(bytes[1], '\x01');
}

TEST(ProtoRoundTrip, TextChatMsgKeepsEveryRepeatedEntry)
{
    message::TextChatMsgReq req;
    req.set_fromuid(11);
    req.set_touid(22);
    for (const char* content : {"first", "second", "third"}) {
        auto* data = req.add_textmsgs();
        data->set_msgid(std::string("id-") + content);
        data->set_msgcontent(content);
    }

    message::TextChatMsgReq decoded;
    ASSERT_TRUE(decoded.ParseFromString(req.SerializeAsString()));

    EXPECT_EQ(decoded.fromuid(), 11);
    EXPECT_EQ(decoded.touid(), 22);
    ASSERT_EQ(decoded.textmsgs_size(), 3);
    EXPECT_EQ(decoded.textmsgs(0).msgcontent(), "first");
    EXPECT_EQ(decoded.textmsgs(2).msgid(), "id-third");
}

TEST(ProtoRoundTrip, EmptyMessageSurvivesRoundTrip)
{
    message::GetVerifyRsp rsp;

    message::GetVerifyRsp decoded;
    ASSERT_TRUE(decoded.ParseFromString(rsp.SerializeAsString()));

    EXPECT_EQ(decoded.error(), 0);
    EXPECT_TRUE(decoded.email().empty());
    EXPECT_TRUE(decoded.code().empty());
}

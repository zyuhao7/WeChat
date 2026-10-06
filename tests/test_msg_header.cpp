#include <gtest/gtest.h>

#include "MsgHeader.h"

TEST(MsgHeader, AcceptsWellFormedFrames)
{
    EXPECT_TRUE(IsValidMsgHeader(MSG_CHAT_LOGIN, 0)); // no body, e.g. a heartbeat
    EXPECT_TRUE(IsValidMsgHeader(ID_TEXT_CHAT_MSG_REQ, 512));
    EXPECT_TRUE(IsValidMsgHeader(1005, MAX_LENGTH)); // exactly at the bound
}

// 0xFFFF on the wire reads back as a signed short -1, which sailed past the old
// `> MAX_LENGTH` test and reached `new char[len + 1]` as a negative array bound.
TEST(MsgHeader, RejectsAHighBitLengthAsSentOnTheWire)
{
    EXPECT_FALSE(IsValidMsgHeader(MSG_CHAT_LOGIN, static_cast<short>(0xFFFF)));
    EXPECT_FALSE(IsValidMsgHeader(MSG_CHAT_LOGIN, static_cast<short>(0x8000)));
    EXPECT_FALSE(IsValidMsgHeader(MSG_CHAT_LOGIN, -1));
    EXPECT_FALSE(IsValidMsgHeader(MSG_CHAT_LOGIN, -2048));
}

TEST(MsgHeader, RejectsAHighBitIdAsSentOnTheWire)
{
    EXPECT_FALSE(IsValidMsgHeader(static_cast<short>(0xFFFF), 16));
    EXPECT_FALSE(IsValidMsgHeader(-1, 16));
}

TEST(MsgHeader, RejectsAnOverlongBody)
{
    EXPECT_FALSE(IsValidMsgHeader(MSG_CHAT_LOGIN, MAX_LENGTH + 1));
}

TEST(MsgHeader, RejectsAnIdBeyondTheBound)
{
    EXPECT_FALSE(IsValidMsgHeader(MAX_LENGTH + 1, 16));
}

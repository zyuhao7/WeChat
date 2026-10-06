#include <gtest/gtest.h>

#include <cstring>
#include <string>

#include "MsgHeader.h"
#include "MsgNode.h"

namespace {

// The reader in CSession hands the two header fields to network_to_host_short, so that is
// how the test reads back what SendNode wrote.
short PeekShort(const char* at)
{
    short raw;
    std::memcpy(&raw, at, sizeof(raw));
    return boost::asio::detail::socket_ops::network_to_host_short(raw);
}

}  // namespace

// The header SendNode writes and the frame IsValidMsgHeader accepts are two halves of the
// same contract, so they have to agree on order and width.
TEST(MsgNodeSerialization, WritesTheHeaderInNetworkByteOrder)
{
    const std::string payload = "hello";
    SendNode node(payload.data(), static_cast<short>(payload.size()), ID_TEXT_CHAT_MSG_REQ);

    EXPECT_EQ(node._total_len, static_cast<short>(payload.size() + HEAD_TOTAL_LEN));
    EXPECT_EQ(PeekShort(node._data), ID_TEXT_CHAT_MSG_REQ);
    EXPECT_EQ(PeekShort(node._data + HEAD_ID_LEN), static_cast<short>(payload.size()));
    EXPECT_EQ(std::string(node._data + HEAD_TOTAL_LEN, payload.size()), payload);
}

TEST(MsgNodeSerialization, TheHeaderItWritesPassesTheFrameCheck)
{
    const std::string payload(MAX_LENGTH, 'x');
    SendNode node(payload.data(), static_cast<short>(payload.size()), ID_TEXT_CHAT_MSG_REQ);

    EXPECT_TRUE(IsValidMsgHeader(PeekShort(node._data), PeekShort(node._data + HEAD_ID_LEN)));
}

TEST(MsgNodeSerialization, AReceiveNodeStartsEmptyAndTerminated)
{
    RecvNode node(16, MSG_CHAT_LOGIN);

    EXPECT_EQ(node._cur_len, 0);
    EXPECT_EQ(node._total_len, 16);
    EXPECT_EQ(node._data[16], '\0');
}

TEST(MsgNodeSerialization, ClearZeroesTheBufferedBytesAndTheLength)
{
    SendNode node("payload", 7, MSG_CHAT_LOGIN);
    node._cur_len = 3;
    node.Clear();

    EXPECT_EQ(node._cur_len, 0);
    for (short i = 0; i < node._total_len; ++i)
        EXPECT_EQ(node._data[i], 0) << "byte " << i;
}

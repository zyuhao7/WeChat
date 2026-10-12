#pragma once

#include "const.h"

// The wire header is two 16-bit network-order fields, read into `short`s in CSession. The
// length is then used as an array bound and the id as a lookup key, so a value whose high
// bit is set — which arrives negative, e.g. 0xFFFF read back as -1 — has to be rejected,
// not just a value that is too large.
inline bool IsValidMsgHeader(short msg_id, short msg_len)
{
    return msg_id >= 0 && msg_id <= MAX_LENGTH && msg_len >= 0 && msg_len <= MAX_LENGTH;
}

// SendNode stores body length plus header in a `short`, so a larger body would wrap negative.
constexpr std::size_t MAX_SEND_BODY_LEN = 32767 - HEAD_TOTAL_LEN;

inline bool IsSendableBodyLen(std::size_t body_len)
{
    return body_len <= MAX_SEND_BODY_LEN;
}

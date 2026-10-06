#pragma once

#include "const.h"

// The wire header is two 16-bit network-order fields, read into `short`s in CSession. The
// length is then used as an array bound and the id as a lookup key, so a value whose high
// bit is set — which arrives negative, e.g. 0xFFFF read back as -1 — has to be rejected,
// not just a value that is too large.
inline bool IsValidMsgHeader(short msg_id, short msg_len)
{
	return msg_id >= 0 && msg_id <= MAX_LENGTH
		&& msg_len >= 0 && msg_len <= MAX_LENGTH;
}

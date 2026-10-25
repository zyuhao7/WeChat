#pragma once

#include <json/json.h>
#include <climits>
#include <memory>
#include <string>

// Request fields come straight off the wire, so every accessor here fails closed instead of
// letting jsoncpp's as*() coerce garbage to 0 or throw on a type mismatch.

inline bool IsPureDigit(const std::string& str)
{
    if (str.empty())
        return false;
    for (unsigned char c : str)
    {
        if (c < '0' || c > '9')
            return false;
    }
    return true;
}

// Parses a non-negative decimal int; rejects empty, signs, spaces and values above INT_MAX.
inline bool ParseNonNegativeInt(const std::string& str, int& out)
{
    if (!IsPureDigit(str))
        return false;
    long long value = 0;
    for (unsigned char c : str)
    {
        value = value * 10 + (c - '0');
        if (value > INT_MAX)
            return false;
    }
    out = static_cast<int>(value);
    return true;
}

inline bool IsValidUid(int uid)
{
    return uid > 0;
}

inline bool ParseJsonObject(const std::string& data, Json::Value& root)
{
    Json::CharReaderBuilder builder;
    std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
    std::string errs;
    Json::Value parsed;
    if (!reader->parse(data.data(), data.data() + data.size(), &parsed, &errs))
        return false;
    if (!parsed.isObject())
        return false;
    root = std::move(parsed);
    return true;
}

inline bool GetIntField(const Json::Value& root, const char* key, int& out)
{
    if (!root.isObject() || !root.isMember(key))
        return false;
    const Json::Value& v = root[key];
    if (!v.isInt())
        return false;
    out = v.asInt();
    return true;
}

inline bool GetStringField(const Json::Value& root, const char* key, std::string& out)
{
    if (!root.isObject() || !root.isMember(key))
        return false;
    const Json::Value& v = root[key];
    if (!v.isString())
        return false;
    out = v.asString();
    return true;
}

// A file travels as a sequence of chunk frames. The sender base64s the raw bytes, so a chunk
// costs 4/3 of its size on the wire; these limits keep one frame inside MAX_LENGTH.
constexpr int FILE_CHUNK_BYTES = 1024;
constexpr int FILE_MAX_BYTES = 100 * 1024 * 1024;
constexpr int FILE_MAX_NAME_BYTES = 128;
constexpr int FILE_CHUNK_BASE64_LEN = ((FILE_CHUNK_BYTES + 2) / 3) * 4;

struct FileChunk
{
    std::string file_id;
    std::string file_name;
    int file_size = 0;
    int index = 0;
    int total = 0;
    std::string data;
};

inline bool IsValidBase64(const std::string& str)
{
    for (unsigned char c : str)
    {
        const bool ok = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') ||
                        c == '+' || c == '/' || c == '=';
        if (!ok)
            return false;
    }
    return true;
}

// A name that arrives here is only ever relayed, never opened as a path, but a name with a
// separator is still a bug or an attack on the receiver, so it is rejected.
inline bool IsValidFileName(const std::string& name)
{
    if (name.empty() || name.size() > FILE_MAX_NAME_BYTES)
        return false;
    if (name == "." || name == "..")
        return false;
    for (unsigned char c : name)
    {
        if (c < 0x20 || c == 0x7f || c == '/' || c == '\\' || c == ':')
            return false;
    }
    return true;
}

inline int FileChunkTotal(int file_size)
{
    if (file_size <= 0)
        return 0;
    return (file_size + FILE_CHUNK_BYTES - 1) / FILE_CHUNK_BYTES;
}

// Validates one chunk frame. `data` is length-checked but never decoded: the server has no
// use for the bytes and a bad chunk then only fails at the receiver, which is where the
// checksum-free design already puts the risk.
inline bool ParseFileChunk(const Json::Value& root, FileChunk& out)
{
    if (!GetStringField(root, "fileid", out.file_id) || !GetStringField(root, "filename", out.file_name))
        return false;
    if (!GetIntField(root, "filesize", out.file_size) || !GetIntField(root, "index", out.index) ||
        !GetIntField(root, "total", out.total))
        return false;
    if (!GetStringField(root, "data", out.data))
        return false;

    if (out.file_id.empty() || out.file_id.size() > 64 || !IsValidFileName(out.file_name))
        return false;
    if (out.file_size <= 0 || out.file_size > FILE_MAX_BYTES)
        return false;
    if (out.total != FileChunkTotal(out.file_size) || out.total <= 0)
        return false;
    if (out.index < 0 || out.index >= out.total)
        return false;
    // every chunk but the last is exactly full, so its encoded length is fixed
    if (out.index + 1 < out.total && out.data.size() != FILE_CHUNK_BASE64_LEN)
        return false;
    if (out.data.empty() || out.data.size() > FILE_CHUNK_BASE64_LEN || out.data.size() % 4 != 0)
        return false;
    return IsValidBase64(out.data);
}

// A text message batch must be a non-empty array of {content: string, msgid: string}. The
// bounds keep a batch inside the 2048-byte receive frame and the message table's columns.
inline bool IsValidTextArray(const Json::Value& arr)
{
    if (!arr.isArray() || arr.empty())
        return false;
    for (const auto& item : arr)
    {
        if (!item.isObject())
            return false;
        if (!item["content"].isString() || !item["msgid"].isString())
            return false;
        const std::string msg_id = item["msgid"].asString();
        if (msg_id.empty() || msg_id.size() > 64)
            return false;
        if (item["content"].asString().size() > 2048)
            return false;
    }
    return true;
}

constexpr int CHAT_HISTORY_MAX_LIMIT = 50;

struct ChatHistoryQuery
{
    int peer_uid = 0;
    long long last_id = 0;
    int limit = CHAT_HISTORY_MAX_LIMIT;
};

// {touid, last_id?, limit?} — last_id is an exclusive upper bound: 0 asks for the newest page,
// and the client passes the oldest id it already has to walk upward through older pages.
inline bool ParseChatHistoryQuery(const Json::Value& root, int self_uid, ChatHistoryQuery& out)
{
    if (!root.isObject())
        return false;

    int peer_uid = 0;
    if (!GetIntField(root, "touid", peer_uid) || !IsValidUid(peer_uid) || peer_uid == self_uid)
        return false;
    out.peer_uid = peer_uid;

    if (root.isMember("last_id"))
    {
        const Json::Value& v = root["last_id"];
        long long last_id = 0;
        if (v.isInt64())
            last_id = v.asInt64();
        else if (v.isInt())
            last_id = v.asInt();
        else if (v.isUInt64())
        {
            const auto unsigned_id = v.asUInt64();
            if (unsigned_id > static_cast<Json::UInt64>(LLONG_MAX))
                return false;
            last_id = static_cast<long long>(unsigned_id);
        }
        else
            return false;
        if (last_id < 0)
            return false;
        out.last_id = last_id;
    }

    if (root.isMember("limit"))
    {
        int limit = 0;
        if (!GetIntField(root, "limit", limit))
            return false;
        if (limit <= 0)
            return false;
        out.limit = limit > CHAT_HISTORY_MAX_LIMIT ? CHAT_HISTORY_MAX_LIMIT : limit;
    }
    return true;
}

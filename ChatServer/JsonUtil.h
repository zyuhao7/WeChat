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

// A text message batch must be a non-empty array of {content: string, msgid: string}.
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
    }
    return true;
}

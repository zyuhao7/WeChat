#pragma once
#include <cstddef>
#include <string>
#include <unordered_map>
#include <json/json.h>

constexpr std::size_t kMaxEmailLength = 254;
constexpr std::size_t kMaxFieldLength = 256;

// Splits "path?query" into the path and url-decoded params. Segments without '=' become
// keys with an empty value, segments with an empty key are dropped, and the last duplicate wins.
void ParseTarget(const std::string& target, std::string& path,
	std::unordered_map<std::string, std::string>& params);

// Accepts only a JSON object with nothing trailing it.
bool ParseJsonObject(const std::string& body, Json::Value& out);

// Requires obj[key] to be a non-empty string of at most maxLen bytes.
bool GetStringField(const Json::Value& obj, const char* key, std::string& out, std::size_t maxLen);

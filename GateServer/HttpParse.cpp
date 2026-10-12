#include "HttpParse.h"
#include "UrlCodec.h"
#include <memory>

void ParseTarget(const std::string& target, std::string& path,
	std::unordered_map<std::string, std::string>& params)
{
	params.clear();
	auto query_pos = target.find('?');
	path = target.substr(0, query_pos);
	if (query_pos == std::string::npos) {
		return;
	}

	std::size_t start = query_pos + 1;
	while (start <= target.size()) {
		auto end = target.find('&', start);
		if (end == std::string::npos) {
			end = target.size();
		}
		auto eq_pos = target.find('=', start);
		std::string key;
		std::string value;
		if (eq_pos != std::string::npos && eq_pos < end) {
			key = UrlDecode(target.substr(start, eq_pos - start));
			value = UrlDecode(target.substr(eq_pos + 1, end - eq_pos - 1));
		}
		else {
			key = UrlDecode(target.substr(start, end - start));
		}
		if (!key.empty()) {
			params[key] = value;
		}
		start = end + 1;
	}
}

bool ParseJsonObject(const std::string& body, Json::Value& out)
{
	Json::CharReaderBuilder builder;
	builder["collectComments"] = false;
	builder["failIfExtra"] = true;
	builder["rejectDupKeys"] = true;
	std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
	std::string errs;
	Json::Value parsed;
	if (!reader->parse(body.data(), body.data() + body.size(), &parsed, &errs)) {
		return false;
	}
	if (!parsed.isObject()) {
		return false;
	}
	out = std::move(parsed);
	return true;
}

bool GetStringField(const Json::Value& obj, const char* key, std::string& out, std::size_t maxLen)
{
	if (!obj.isObject() || !obj.isMember(key)) {
		return false;
	}
	const Json::Value& v = obj[key];
	if (!v.isString()) {
		return false;
	}
	std::string s = v.asString();
	if (s.empty() || s.size() > maxLen) {
		return false;
	}
	out = std::move(s);
	return true;
}

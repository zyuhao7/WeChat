#pragma once

#include <memory>
#include <string>
#include <json/json.h>
#include "data.h"
#include "JsonUtil.h"

// Redis caches the whole user row (pwd included) for server-side lookups; only
// FillPublicUserInfo decides what reaches a client, and it never copies pwd.

inline bool ParseCachedUserInfo(const std::string& cached, UserInfo& info)
{
	Json::Value root;
	if (!ParseJsonObject(cached, root))
		return false;
	int uid = 0;
	if (!GetIntField(root, "uid", uid) || !IsValidUid(uid))
		return false;
	auto str = [&root](const char* key) {
		return root[key].isString() ? root[key].asString() : std::string();
	};
	info.uid = uid;
	info.name = str("name");
	info.pwd = str("pwd");
	info.email = str("email");
	info.nick = str("nick");
	info.desc = str("desc");
	info.icon = str("icon");
	info.sex = root["sex"].isInt() ? root["sex"].asInt() : 0;
	return true;
}

inline std::string SerializeUserInfoForCache(const UserInfo& info)
{
	Json::Value root;
	root["uid"] = info.uid;
	root["pwd"] = info.pwd;
	root["name"] = info.name;
	root["email"] = info.email;
	root["nick"] = info.nick;
	root["desc"] = info.desc;
	root["sex"] = info.sex;
	root["icon"] = info.icon;
	return root.toStyledString();
}

inline void FillPublicUserInfo(const UserInfo& info, Json::Value& out)
{
	out["uid"] = info.uid;
	out["name"] = info.name;
	out["email"] = info.email;
	out["nick"] = info.nick;
	out["desc"] = info.desc;
	out["sex"] = info.sex;
	out["icon"] = info.icon;
}

// Redis first, then MySQL (refilling the cache); false if the user does not exist.
bool LoadUserInfoByUid(int uid, std::shared_ptr<UserInfo>& info);
bool LoadUserInfoByName(const std::string& name, std::shared_ptr<UserInfo>& info);

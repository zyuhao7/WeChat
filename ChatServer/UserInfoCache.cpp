#include "UserInfoCache.h"
#include "const.h"
#include "MysqlMgr.h"
#include "RedisMgr.h"

namespace
{

// A corrupt or stale cache entry falls through to MySQL instead of yielding a uid-0 user.
bool LoadFromCache(const std::string& key, std::shared_ptr<UserInfo>& info)
{
    std::string cached;
    if (!RedisMgr::GetInstance()->Get(key, cached))
        return false;
    auto parsed = std::make_shared<UserInfo>();
    if (!ParseCachedUserInfo(cached, *parsed))
    {
        std::cout << "drop malformed user cache entry " << key << std::endl;
        return false;
    }
    info = parsed;
    return true;
}

} // namespace

bool LoadUserInfoByUid(int uid, std::shared_ptr<UserInfo>& info)
{
    if (!IsValidUid(uid))
        return false;
    std::string key = USER_BASE_INFO + std::to_string(uid);
    if (LoadFromCache(key, info) && info->uid == uid)
        return true;

    auto user_info = MysqlMgr::GetInstance()->GetUser(uid);
    if (user_info == nullptr)
        return false;
    info = user_info;
    RedisMgr::GetInstance()->Set(key, SerializeUserInfoForCache(*info));
    return true;
}

bool LoadUserInfoByName(const std::string& name, std::shared_ptr<UserInfo>& info)
{
    if (name.empty())
        return false;
    std::string key = NAME_INFO + name;
    if (LoadFromCache(key, info) && info->name == name)
        return true;

    auto user_info = MysqlMgr::GetInstance()->GetUser(name);
    if (user_info == nullptr)
        return false;
    info = user_info;
    RedisMgr::GetInstance()->Set(key, SerializeUserInfoForCache(*info));
    return true;
}

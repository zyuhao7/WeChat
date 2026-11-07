#pragma once

#include <drogon/drogon.h>

#include <functional>
#include <string>

#include "Singleton.h"

class LogicSystem : public Singleton<LogicSystem>
{
    friend class Singleton<LogicSystem>;

public:
    ~LogicSystem() = default;
    void RegisterHandlers();

private:
    using Callback = std::function<void(const drogon::HttpResponsePtr&)>;
    using Handler = std::function<void(const drogon::HttpRequestPtr&, const Callback&)>;

    LogicSystem() = default;
    void RegGet(const std::string& path, Handler handler);
    void RegPost(const std::string& path, Handler handler);
};

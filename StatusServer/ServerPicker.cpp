#include "ServerPicker.h"
#include <algorithm>
#include <charconv>

namespace
{
std::string trim(const std::string& s)
{
    const char* blanks = " \t\r\n";
    auto begin = s.find_first_not_of(blanks);
    if (begin == std::string::npos)
    {
        return "";
    }
    auto end = s.find_last_not_of(blanks);
    return s.substr(begin, end - begin + 1);
}
} // namespace

std::vector<std::string> ParseServerList(const std::string& list)
{
    std::vector<std::string> names;
    std::string::size_type start = 0;
    while (start <= list.size())
    {
        auto comma = list.find(',', start);
        if (comma == std::string::npos)
        {
            comma = list.size();
        }
        auto name = trim(list.substr(start, comma - start));
        if (!name.empty() && std::find(names.begin(), names.end(), name) == names.end())
        {
            names.push_back(name);
        }
        start = comma + 1;
    }
    return names;
}

std::optional<int> ParseLoginCount(const std::string& text)
{
    int value = 0;
    const char* first = text.data();
    const char* last = first + text.size();
    auto [ptr, ec] = std::from_chars(first, last, value);
    if (ec != std::errc() || ptr != last || value < 0)
    {
        return std::nullopt;
    }
    return value;
}

std::optional<std::string> PickServer(const std::vector<std::string>& names,
                                      const std::map<std::string, std::string>& counts)
{
    std::optional<std::string> best;
    int best_count = 0;
    for (const auto& name : names)
    {
        auto it = counts.find(name);
        if (it == counts.end())
        {
            continue;
        }
        auto count = ParseLoginCount(it->second);
        if (!count)
        {
            continue;
        }
        if (!best || *count < best_count)
        {
            best = name;
            best_count = *count;
        }
    }
    return best;
}

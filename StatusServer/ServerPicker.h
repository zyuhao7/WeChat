#pragma once
#include <map>
#include <optional>
#include <string>
#include <vector>

// Splits the comma-separated chatservers list; trims blanks, drops empty and duplicate names.
std::vector<std::string> ParseServerList(const std::string& list);

// Parses a login count stored in Redis; rejects empty, non-numeric, negative and out-of-range values.
std::optional<int> ParseLoginCount(const std::string& text);

// Picks the server with the fewest logins. Servers whose count is missing or unparsable are
// treated as unavailable; ties go to the earliest name in `names`.
std::optional<std::string> PickServer(const std::vector<std::string>& names,
                                      const std::map<std::string, std::string>& counts);

#pragma once

#include <string>

// Percent-encoding helpers used by the GET-parameter parser.

unsigned char ToHex(unsigned char x);

// Returns the value of a hex digit, or -1 when x is not one.
int FromHex(unsigned char x);

std::string UrlEncode(const std::string& str);
std::string UrlDecode(const std::string& str);

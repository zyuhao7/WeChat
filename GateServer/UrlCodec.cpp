#include "UrlCodec.h"

#include <cctype>

unsigned char ToHex(unsigned char x)
{
	return x > 9 ? x + 55 : x + 48;
}

int FromHex(unsigned char x)
{
	if (x >= 'A' && x <= 'F') return x - 'A' + 10;
	if (x >= 'a' && x <= 'f') return x - 'a' + 10;
	if (x >= '0' && x <= '9') return x - '0';
	return -1;
}

std::string UrlEncode(const std::string& str)
{
	std::string strTemp;
	const size_t length = str.length();
	for (size_t i = 0; i < length; i++)
	{
		//check whether it consists only of digits and letters
		if (isalnum((unsigned char)str[i]) ||
			(str[i] == '-') ||
			(str[i] == '_') ||
			(str[i] == '.') ||
			(str[i] == '~'))
			strTemp += str[i];
		else if (str[i] == ' ') //empty character
			strTemp += "+";
		else
		{
			//other chars need a leading % and their high and low nibbles converted to hex
			strTemp += '%';
			strTemp += ToHex((unsigned char)str[i] >> 4);
			strTemp += ToHex((unsigned char)str[i] & 0x0F);
		}
	}
	return strTemp;
}

std::string UrlDecode(const std::string& str)
{
	std::string strTemp;
	strTemp.reserve(str.length());
	const size_t length = str.length();
	for (size_t i = 0; i < length; i++)
	{
		//restore + to empty
		if (str[i] == '+')
		{
			strTemp += ' ';
			continue;
		}

		//a complete escape needs two more characters; without them the % is literal
		if (str[i] == '%' && i + 2 < length)
		{
			const int high = FromHex((unsigned char)str[i + 1]);
			const int low = FromHex((unsigned char)str[i + 2]);
			if (high >= 0 && low >= 0)
			{
				strTemp += static_cast<char>(high * 16 + low);
				i += 2;
				continue;
			}
		}

		strTemp += str[i];
	}
	return strTemp;
}

#include <gtest/gtest.h>

#include <string>

#include "UrlCodec.h"

TEST(UrlDecode, DecodesPercentEscapes)
{
    EXPECT_EQ(UrlDecode("%41"), "A");
    EXPECT_EQ(UrlDecode("a%20b"), "a b");
    EXPECT_EQ(UrlDecode("%7E%7e"), "~~");
    EXPECT_EQ(UrlDecode(""), "");
}

TEST(UrlDecode, PlusBecomesSpace)
{
    EXPECT_EQ(UrlDecode("a+b"), "a b");
    EXPECT_EQ(UrlDecode("+"), " ");
}

TEST(UrlDecode, LeavesPlainTextAlone)
{
    EXPECT_EQ(UrlDecode("name=value"), "name=value");
}

// A truncated escape walked off the end of the string: the guard was an assert, and the
// Release build the servers ship with compiles asserts out.
TEST(UrlDecode, TruncatedEscapeStaysLiteral)
{
    EXPECT_EQ(UrlDecode("%"), "%");
    EXPECT_EQ(UrlDecode("%A"), "%A");
    EXPECT_EQ(UrlDecode("a%2"), "a%2");
    EXPECT_EQ(UrlDecode("100%"), "100%");
}

TEST(UrlDecode, InvalidHexDigitsStayLiteral)
{
    EXPECT_EQ(UrlDecode("%ZZ"), "%ZZ");
    EXPECT_EQ(UrlDecode("%G1"), "%G1");
    EXPECT_EQ(UrlDecode("%2G"), "%2G");
}

TEST(UrlDecode, SurvivesAStringOfBarePercents)
{
    EXPECT_EQ(UrlDecode(std::string(64, '%')), std::string(64, '%'));
}

TEST(FromHex, RejectsNonHexDigits)
{
    EXPECT_EQ(FromHex('0'), 0);
    EXPECT_EQ(FromHex('9'), 9);
    EXPECT_EQ(FromHex('a'), 10);
    EXPECT_EQ(FromHex('F'), 15);

    // previously accepted as 10..35, which overflowed the decoded byte
    EXPECT_EQ(FromHex('G'), -1);
    EXPECT_EQ(FromHex('z'), -1);
    EXPECT_EQ(FromHex('%'), -1);
}

TEST(UrlCodec, RoundTripsThroughEncodeThenDecode)
{
    for (const char* original : {"a b", "a+b", "100%", "\xe4\xb8\xad\xe6\x96\x87", "/path?x=1&y=2", "%"})
    {
        EXPECT_EQ(UrlDecode(UrlEncode(original)), original) << "original: " << original;
    }
}

TEST(UrlEncode, KeepsUnreservedCharacters)
{
    EXPECT_EQ(UrlEncode("AZaz09-_.~"), "AZaz09-_.~");
    EXPECT_EQ(UrlEncode(""), "");
}

TEST(UrlEncode, SpaceBecomesPlus)
{
    EXPECT_EQ(UrlEncode("a b"), "a+b");
}

TEST(UrlEncode, EscapesReservedCharactersUppercase)
{
    EXPECT_EQ(UrlEncode("&=?/#+%"), "%26%3D%3F%2F%23%2B%25");
}

TEST(UrlEncode, EscapesControlAndHighBytes)
{
    EXPECT_EQ(UrlEncode(std::string(1, '\0')), "%00");
    EXPECT_EQ(UrlEncode("\x7f"), "%7F");
    EXPECT_EQ(UrlEncode("\x80"), "%80");
    EXPECT_EQ(UrlEncode("\xff"), "%FF");
    EXPECT_EQ(UrlEncode("\xe4\xb8\xad"), "%E4%B8%AD");
}

TEST(UrlCodec, RoundTripsEveryByte)
{
    std::string all;
    for (int c = 0; c < 256; ++c)
    {
        all.push_back(static_cast<char>(c));
    }
    EXPECT_EQ(UrlDecode(UrlEncode(all)), all);
}

TEST(UrlDecode, DecodesNulAndHighBytes)
{
    EXPECT_EQ(UrlDecode("%00"), std::string(1, '\0'));
    EXPECT_EQ(UrlDecode("%ff%FF"), "\xff\xff");
}

TEST(ToHex, CoversTheNibbleRange)
{
    EXPECT_EQ(ToHex(0), '0');
    EXPECT_EQ(ToHex(9), '9');
    EXPECT_EQ(ToHex(10), 'A');
    EXPECT_EQ(ToHex(15), 'F');
}

TEST(FromHex, InvertsToHexAndRejectsNeighbours)
{
    for (unsigned char x = 0; x < 16; ++x)
    {
        EXPECT_EQ(FromHex(ToHex(x)), x);
    }
    EXPECT_EQ(FromHex('f'), 15);
    // the characters just outside each hex range
    for (unsigned char c : {'/', ':', '@', 'G', '`', 'g', '\0', '\x7f', '\x80', '\xff'})
    {
        EXPECT_EQ(FromHex(c), -1) << "char code " << static_cast<int>(c);
    }
}

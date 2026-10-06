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
    for (const char* original : {"a b", "a+b", "100%", "\xe4\xb8\xad\xe6\x96\x87", "/path?x=1&y=2", "%"}) {
        EXPECT_EQ(UrlDecode(UrlEncode(original)), original) << "original: " << original;
    }
}

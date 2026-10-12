#include <gtest/gtest.h>

#include <string>
#include <unordered_map>

#include "HttpParse.h"

namespace
{

using Params = std::unordered_map<std::string, std::string>;

Params Parse(const std::string& target, std::string* path_out = nullptr)
{
    std::string path;
    Params params;
    ParseTarget(target, path, params);
    if (path_out)
    {
        *path_out = path;
    }
    return params;
}

} // namespace

TEST(ParseTarget, NoQueryKeepsWholePath)
{
    std::string path;
    EXPECT_TRUE(Parse("/get_test", &path).empty());
    EXPECT_EQ(path, "/get_test");
}

TEST(ParseTarget, TrailingQuestionMarkIsStripped)
{
    std::string path;
    EXPECT_TRUE(Parse("/user_login?", &path).empty());
    EXPECT_EQ(path, "/user_login");
}

TEST(ParseTarget, PostTargetWithQueryResolvesToBarePath)
{
    std::string path;
    Parse("/user_login?v=2", &path);
    EXPECT_EQ(path, "/user_login");
}

TEST(ParseTarget, SimplePairs)
{
    auto p = Parse("/t?name=123&age=456");
    EXPECT_EQ(p.size(), 2u);
    EXPECT_EQ(p["name"], "123");
    EXPECT_EQ(p["age"], "456");
}

TEST(ParseTarget, EmptySegmentsAreSkipped)
{
    auto p = Parse("/t?a=1&&b=2");
    EXPECT_EQ(p.size(), 2u);
    EXPECT_EQ(p["a"], "1");
    EXPECT_EQ(p["b"], "2");
    EXPECT_TRUE(Parse("/t?&").empty());
    EXPECT_TRUE(Parse("/t?&&&").empty());
}

TEST(ParseTarget, TrailingAmpersand)
{
    auto p = Parse("/t?a=1&");
    EXPECT_EQ(p.size(), 1u);
    EXPECT_EQ(p["a"], "1");
}

TEST(ParseTarget, EmptyKeyIsDropped)
{
    EXPECT_TRUE(Parse("/t?=").empty());
    EXPECT_TRUE(Parse("/t?=v").empty());
}

TEST(ParseTarget, EmptyValueIsKept)
{
    auto p = Parse("/t?a=");
    ASSERT_EQ(p.count("a"), 1u);
    EXPECT_EQ(p["a"], "");
}

TEST(ParseTarget, KeyWithoutEqualsHasEmptyValue)
{
    auto p = Parse("/t?flag&a=1");
    EXPECT_EQ(p.size(), 2u);
    ASSERT_EQ(p.count("flag"), 1u);
    EXPECT_EQ(p["flag"], "");
    EXPECT_EQ(p["a"], "1");
}

TEST(ParseTarget, OnlyFirstEqualsSplits)
{
    auto p = Parse("/t?a=b=c");
    EXPECT_EQ(p["a"], "b=c");
}

TEST(ParseTarget, LastDuplicateWins)
{
    auto p = Parse("/t?a=1&a=2");
    EXPECT_EQ(p.size(), 1u);
    EXPECT_EQ(p["a"], "2");
}

TEST(ParseTarget, EncodedSeparatorsStayInsideTheValue)
{
    auto p = Parse("/t?q=x%26y%3Dz&n=1");
    EXPECT_EQ(p.size(), 2u);
    EXPECT_EQ(p["q"], "x&y=z");
    EXPECT_EQ(p["n"], "1");
}

TEST(ParseTarget, PlusDecodesToSpace)
{
    auto p = Parse("/t?full+name=a+b");
    EXPECT_EQ(p["full name"], "a b");
}

TEST(ParseTarget, Utf8RawAndEncoded)
{
    auto p = Parse("/t?raw=\xe4\xb8\xad&enc=%E4%B8%AD");
    EXPECT_EQ(p["raw"], "\xe4\xb8\xad");
    EXPECT_EQ(p["enc"], "\xe4\xb8\xad");
}

TEST(ParseTarget, ResetsParamsFromAPreviousCall)
{
    std::string path;
    Params params{{"stale", "1"}};
    ParseTarget("/t?a=1", path, params);
    EXPECT_EQ(params.count("stale"), 0u);
}

TEST(ParseTarget, HandlesA64KbQuery)
{
    std::string value(64 * 1024, 'x');
    auto p = Parse("/t?big=" + value + "&a=1");
    EXPECT_EQ(p["big"], value);
    EXPECT_EQ(p["a"], "1");
}

TEST(ParseJsonObject, RejectsNonObjects)
{
    Json::Value v;
    for (const char* body : {"", "null", "[]", "\"str\"", "1", "true", "   ", "{", "{\"a\":1,}x"})
    {
        EXPECT_FALSE(ParseJsonObject(body, v)) << "body: " << body;
    }
}

TEST(ParseJsonObject, RejectsTrailingGarbage)
{
    Json::Value v;
    EXPECT_FALSE(ParseJsonObject("{\"email\":\"a@b.cn\"} trailing", v));
    EXPECT_FALSE(ParseJsonObject("{}{}", v));
}

TEST(ParseJsonObject, AcceptsObjects)
{
    Json::Value v;
    EXPECT_TRUE(ParseJsonObject("{}", v));
    EXPECT_TRUE(v.isObject());
    EXPECT_TRUE(ParseJsonObject(" {\"email\":\"a@b.cn\"}\n", v));
    EXPECT_EQ(v["email"].asString(), "a@b.cn");
}

TEST(ParseJsonObject, FailureLeavesOutputUntouched)
{
    Json::Value v(Json::objectValue);
    v["keep"] = 1;
    EXPECT_FALSE(ParseJsonObject("[]", v));
    EXPECT_EQ(v["keep"].asInt(), 1);
}

namespace
{

bool Field(const std::string& body, std::string& out, std::size_t maxLen = kMaxEmailLength)
{
    Json::Value root;
    if (!ParseJsonObject(body, root))
    {
        return false;
    }
    return GetStringField(root, "email", out, maxLen);
}

} // namespace

TEST(GetStringField, RejectsWrongTypes)
{
    std::string out;
    for (const char* body : {"{\"email\":{}}", "{\"email\":[]}", "{\"email\":1}", "{\"email\":true}",
                             "{\"email\":null}", "{\"email\":[\"a@b.cn\"]}"})
    {
        EXPECT_FALSE(Field(body, out)) << "body: " << body;
    }
}

TEST(GetStringField, RejectsMissingAndEmpty)
{
    std::string out;
    EXPECT_FALSE(Field("{}", out));
    EXPECT_FALSE(Field("{\"Email\":\"a@b.cn\"}", out));
    EXPECT_FALSE(Field("{\"email\":\"\"}", out));
}

TEST(GetStringField, RejectsNonObjectRoot)
{
    std::string out;
    EXPECT_FALSE(GetStringField(Json::Value(Json::arrayValue), "email", out, kMaxEmailLength));
    EXPECT_FALSE(GetStringField(Json::Value("a@b.cn"), "email", out, kMaxEmailLength));
    EXPECT_FALSE(GetStringField(Json::Value(), "email", out, kMaxEmailLength));
}

TEST(GetStringField, EnforcesTheLengthLimitInclusively)
{
    std::string at_limit(kMaxEmailLength, 'a');
    std::string over_limit(kMaxEmailLength + 1, 'a');
    std::string out;
    EXPECT_TRUE(Field("{\"email\":\"" + at_limit + "\"}", out));
    EXPECT_EQ(out, at_limit);
    EXPECT_FALSE(Field("{\"email\":\"" + over_limit + "\"}", out));
    EXPECT_FALSE(Field("{\"email\":\"" + std::string(64 * 1024, 'a') + "\"}", out));
}

TEST(GetStringField, ReturnsTheValue)
{
    std::string out;
    EXPECT_TRUE(Field("{\"email\":\"\xe4\xb8\xad@b.cn\",\"other\":1}", out));
    EXPECT_EQ(out, "\xe4\xb8\xad@b.cn");
}

TEST(GetStringField, LeavesOutputUntouchedOnFailure)
{
    std::string out = "previous";
    EXPECT_FALSE(Field("{\"email\":1}", out));
    EXPECT_EQ(out, "previous");
}

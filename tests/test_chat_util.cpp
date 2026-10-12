#include <gtest/gtest.h>

#include "JsonUtil.h"
#include "MsgHeader.h"
#include "UserInfoCache.h"

TEST(IsPureDigit, Boundaries)
{
    EXPECT_FALSE(IsPureDigit(""));
    EXPECT_TRUE(IsPureDigit("0"));
    EXPECT_TRUE(IsPureDigit("0123"));
    EXPECT_FALSE(IsPureDigit("12a"));
    EXPECT_FALSE(IsPureDigit(" 1"));
    EXPECT_FALSE(IsPureDigit("1 "));
    EXPECT_FALSE(IsPureDigit("-1"));
    EXPECT_FALSE(IsPureDigit("+1"));
    EXPECT_FALSE(IsPureDigit("\xe4\xb8\x80"));
    EXPECT_FALSE(IsPureDigit(std::string("1\0" "2", 3)));
}

TEST(ParseNonNegativeInt, Range)
{
    int out = -1;
    EXPECT_TRUE(ParseNonNegativeInt("0", out));
    EXPECT_EQ(out, 0);
    EXPECT_TRUE(ParseNonNegativeInt("2147483647", out));
    EXPECT_EQ(out, 2147483647);
    EXPECT_TRUE(ParseNonNegativeInt("000042", out));
    EXPECT_EQ(out, 42);

    out = 7;
    EXPECT_FALSE(ParseNonNegativeInt("2147483648", out));
    EXPECT_FALSE(ParseNonNegativeInt("99999999999999999999", out));
    EXPECT_FALSE(ParseNonNegativeInt("", out));
    EXPECT_FALSE(ParseNonNegativeInt("-5", out));
    EXPECT_FALSE(ParseNonNegativeInt("1e3", out));
    EXPECT_EQ(out, 7);
}

TEST(IsValidUid, Sign)
{
    EXPECT_FALSE(IsValidUid(0));
    EXPECT_FALSE(IsValidUid(-1));
    EXPECT_TRUE(IsValidUid(1));
    EXPECT_TRUE(IsValidUid(INT_MAX));
}

TEST(ParseJsonObject, RejectsNonObjects)
{
    Json::Value root;
    EXPECT_FALSE(ParseJsonObject("", root));
    EXPECT_FALSE(ParseJsonObject("{", root));
    EXPECT_FALSE(ParseJsonObject("not json", root));
    EXPECT_FALSE(ParseJsonObject("[1,2]", root));
    EXPECT_FALSE(ParseJsonObject("42", root));
    EXPECT_FALSE(ParseJsonObject("\"str\"", root));
    EXPECT_FALSE(ParseJsonObject("null", root));
    EXPECT_TRUE(ParseJsonObject("{}", root));
    EXPECT_TRUE(ParseJsonObject("{\"uid\":1}", root));
    EXPECT_EQ(root["uid"].asInt(), 1);
}

TEST(GetIntField, TypeAndRange)
{
    Json::Value root;
    ASSERT_TRUE(ParseJsonObject(
        R"({"ok":5,"neg":-3,"str":"5","dbl":1.5,"big":3000000000,"nul":null,"arr":[1]})", root));
    int out = 0;
    EXPECT_TRUE(GetIntField(root, "ok", out));
    EXPECT_EQ(out, 5);
    EXPECT_TRUE(GetIntField(root, "neg", out));
    EXPECT_EQ(out, -3);
    EXPECT_FALSE(GetIntField(root, "str", out));
    EXPECT_FALSE(GetIntField(root, "dbl", out));
    EXPECT_FALSE(GetIntField(root, "big", out));
    EXPECT_FALSE(GetIntField(root, "nul", out));
    EXPECT_FALSE(GetIntField(root, "arr", out));
    EXPECT_FALSE(GetIntField(root, "missing", out));
    EXPECT_FALSE(GetIntField(Json::Value(Json::arrayValue), "ok", out));
}

TEST(GetStringField, Type)
{
    Json::Value root;
    ASSERT_TRUE(ParseJsonObject(R"({"token":"abc","empty":"","num":1})", root));
    std::string out = "x";
    EXPECT_TRUE(GetStringField(root, "token", out));
    EXPECT_EQ(out, "abc");
    EXPECT_TRUE(GetStringField(root, "empty", out));
    EXPECT_EQ(out, "");
    EXPECT_FALSE(GetStringField(root, "num", out));
    EXPECT_FALSE(GetStringField(root, "missing", out));
}

TEST(IsValidTextArray, Shapes)
{
    auto parse = [](const char* text) {
        Json::Value root;
        EXPECT_TRUE(ParseJsonObject(text, root));
        return root["text_array"];
    };
    EXPECT_TRUE(IsValidTextArray(parse(R"({"text_array":[{"content":"hi","msgid":"m1"}]})")));
    EXPECT_TRUE(IsValidTextArray(parse(R"({"text_array":[{"content":"","msgid":"m1"},{"content":"b","msgid":"m2"}]})")));
    EXPECT_FALSE(IsValidTextArray(parse(R"({"text_array":[]})")));
    EXPECT_FALSE(IsValidTextArray(parse(R"({"text_array":{}})")));
    EXPECT_FALSE(IsValidTextArray(parse(R"({"text_array":"hi"})")));
    EXPECT_FALSE(IsValidTextArray(parse(R"({})")));
    EXPECT_FALSE(IsValidTextArray(parse(R"({"text_array":[1]})")));
    EXPECT_FALSE(IsValidTextArray(parse(R"({"text_array":[{"content":"hi"}]})")));
    EXPECT_FALSE(IsValidTextArray(parse(R"({"text_array":[{"content":"hi","msgid":7}]})")));
    EXPECT_FALSE(IsValidTextArray(parse(R"({"text_array":[{"content":null,"msgid":"m"}]})")));
    EXPECT_FALSE(IsValidTextArray(parse(R"({"text_array":[{"content":"a","msgid":"m"},"bad"]})")));
}

TEST(SendBodyLen, FitsSignedShortFrame)
{
    EXPECT_EQ(MAX_SEND_BODY_LEN + HEAD_TOTAL_LEN, 32767u);
    EXPECT_TRUE(IsSendableBodyLen(0));
    EXPECT_TRUE(IsSendableBodyLen(MAX_SEND_BODY_LEN));
    EXPECT_FALSE(IsSendableBodyLen(MAX_SEND_BODY_LEN + 1));
    EXPECT_FALSE(IsSendableBodyLen(65536));
}

TEST(UserInfoCache, RoundTrip)
{
    UserInfo in;
    in.uid = 1019;
    in.name = "alice";
    in.pwd = "secret";
    in.email = "a@test.com";
    in.nick = "Al";
    in.desc = "d";
    in.sex = 1;
    in.icon = ":/res/head_1.jpg";

    UserInfo out;
    ASSERT_TRUE(ParseCachedUserInfo(SerializeUserInfoForCache(in), out));
    EXPECT_EQ(out.uid, in.uid);
    EXPECT_EQ(out.name, in.name);
    EXPECT_EQ(out.pwd, in.pwd);
    EXPECT_EQ(out.email, in.email);
    EXPECT_EQ(out.nick, in.nick);
    EXPECT_EQ(out.desc, in.desc);
    EXPECT_EQ(out.sex, in.sex);
    EXPECT_EQ(out.icon, in.icon);
}

TEST(UserInfoCache, RejectsBadEntries)
{
    UserInfo out;
    EXPECT_FALSE(ParseCachedUserInfo("", out));
    EXPECT_FALSE(ParseCachedUserInfo("garbage", out));
    EXPECT_FALSE(ParseCachedUserInfo("[]", out));
    EXPECT_FALSE(ParseCachedUserInfo(R"({"name":"a"})", out));
    EXPECT_FALSE(ParseCachedUserInfo(R"({"uid":0})", out));
    EXPECT_FALSE(ParseCachedUserInfo(R"({"uid":-4})", out));
    EXPECT_FALSE(ParseCachedUserInfo(R"({"uid":"12"})", out));
}

TEST(UserInfoCache, MissingOrWrongTypedFieldsDefault)
{
    UserInfo out;
    out.name = "stale";
    out.sex = 9;
    ASSERT_TRUE(ParseCachedUserInfo(R"({"uid":3,"name":5,"sex":"x"})", out));
    EXPECT_EQ(out.uid, 3);
    EXPECT_EQ(out.name, "");
    EXPECT_EQ(out.sex, 0);
    EXPECT_EQ(out.icon, "");
}

TEST(UserInfoCache, PublicInfoNeverCarriesPassword)
{
    UserInfo in;
    in.uid = 7;
    in.name = "bob";
    in.pwd = "hunter2";
    Json::Value out;
    FillPublicUserInfo(in, out);
    EXPECT_FALSE(out.isMember("pwd"));
    EXPECT_EQ(out["uid"].asInt(), 7);
    EXPECT_EQ(out["name"].asString(), "bob");
    EXPECT_EQ(out.toStyledString().find("hunter2"), std::string::npos);
}

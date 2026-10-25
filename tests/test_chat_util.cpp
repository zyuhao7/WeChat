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
    EXPECT_FALSE(IsPureDigit(std::string("1\0"
                                         "2",
                                         3)));
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

TEST(IsValidTextArray, Bounds)
{
    Json::Value text_array(Json::arrayValue);
    Json::Value item;
    item["msgid"] = std::string(64, 'a');
    item["content"] = "hi";
    text_array.append(item);

    EXPECT_TRUE(IsValidTextArray(text_array));

    text_array[0]["msgid"] = std::string(65, 'a');
    EXPECT_FALSE(IsValidTextArray(text_array));

    text_array[0]["msgid"] = "";
    EXPECT_FALSE(IsValidTextArray(text_array));

    text_array[0]["msgid"] = "m1";
    text_array[0]["content"] = std::string(2049, 'x');
    EXPECT_FALSE(IsValidTextArray(text_array));
}

TEST(ParseChatHistoryQuery, FieldsAndBoundaries)
{
    auto parse = [](const char* text) {
        Json::Value root;
        EXPECT_TRUE(ParseJsonObject(text, root));
        return root;
    };

    ChatHistoryQuery query;
    EXPECT_TRUE(ParseChatHistoryQuery(parse(R"({"touid":3})"), 1, query));
    EXPECT_EQ(query.peer_uid, 3);
    EXPECT_EQ(query.last_id, 0);
    EXPECT_EQ(query.limit, CHAT_HISTORY_MAX_LIMIT);

    EXPECT_TRUE(ParseChatHistoryQuery(parse(R"({"touid":3,"last_id":42,"limit":10})"), 1, query));
    EXPECT_EQ(query.last_id, 42);
    EXPECT_EQ(query.limit, 10);

    // a limit above the cap is clamped instead of rejected
    EXPECT_TRUE(ParseChatHistoryQuery(parse(R"({"touid":3,"limit":1000})"), 1, query));
    EXPECT_EQ(query.limit, CHAT_HISTORY_MAX_LIMIT);

    EXPECT_FALSE(ParseChatHistoryQuery(parse(R"({})"), 1, query));
    EXPECT_FALSE(ParseChatHistoryQuery(parse(R"({"touid":1})"), 1, query));
    EXPECT_FALSE(ParseChatHistoryQuery(parse(R"({"touid":0})"), 1, query));
    EXPECT_FALSE(ParseChatHistoryQuery(parse(R"({"touid":3,"last_id":-1})"), 1, query));
    EXPECT_FALSE(ParseChatHistoryQuery(parse(R"({"touid":3,"last_id":"9"})"), 1, query));
    EXPECT_FALSE(ParseChatHistoryQuery(parse(R"({"touid":3,"limit":0})"), 1, query));
    EXPECT_FALSE(ParseChatHistoryQuery(parse(R"({"touid":3,"limit":"9"})"), 1, query));
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

TEST(FileChunkTotal, RoundsUpToWholeChunks)
{
    EXPECT_EQ(FileChunkTotal(0), 0);
    EXPECT_EQ(FileChunkTotal(-1), 0);
    EXPECT_EQ(FileChunkTotal(1), 1);
    EXPECT_EQ(FileChunkTotal(FILE_CHUNK_BYTES), 1);
    EXPECT_EQ(FileChunkTotal(FILE_CHUNK_BYTES + 1), 2);
    EXPECT_EQ(FileChunkTotal(FILE_MAX_BYTES), FILE_MAX_BYTES / FILE_CHUNK_BYTES);
}

TEST(IsValidFileName, RejectsSeparatorsAndEmpty)
{
    EXPECT_TRUE(IsValidFileName("report.pdf"));
    EXPECT_TRUE(IsValidFileName("a b.c"));
    EXPECT_TRUE(IsValidFileName(std::string(FILE_MAX_NAME_BYTES, 'x')));
    EXPECT_FALSE(IsValidFileName(""));
    EXPECT_FALSE(IsValidFileName(std::string(FILE_MAX_NAME_BYTES + 1, 'x')));
    EXPECT_FALSE(IsValidFileName("."));
    EXPECT_FALSE(IsValidFileName(".."));
    EXPECT_FALSE(IsValidFileName("a/b"));
    EXPECT_FALSE(IsValidFileName("C:\\a"));
    EXPECT_FALSE(IsValidFileName("a:b"));
    EXPECT_FALSE(IsValidFileName(std::string("a\0b", 3)));
    EXPECT_FALSE(IsValidFileName("a\nb"));
    EXPECT_FALSE(IsValidFileName("a\tb"));
}

TEST(IsValidBase64, AlphabetOnly)
{
    EXPECT_TRUE(IsValidBase64("QUJD"));
    EXPECT_TRUE(IsValidBase64("+/=="));
    EXPECT_TRUE(IsValidBase64(""));
    EXPECT_FALSE(IsValidBase64("QUJD\n"));
    EXPECT_FALSE(IsValidBase64("AB CD"));
    EXPECT_FALSE(IsValidBase64("QU*D"));
}

namespace
{
// A frame that passes every check, so each test can break exactly one field.
Json::Value MakeChunk()
{
    Json::Value root;
    root["fileid"] = "0f8c1e6a";
    root["filename"] = "report.pdf";
    root["filesize"] = 4 * FILE_CHUNK_BYTES + 17;
    root["index"] = 0;
    root["total"] = 5;
    root["data"] = std::string(FILE_CHUNK_BASE64_LEN, 'A');
    return root;
}
} // namespace

TEST(ParseFileChunk, AcceptsAFullChunk)
{
    FileChunk chunk;
    EXPECT_TRUE(ParseFileChunk(MakeChunk(), chunk));
    EXPECT_EQ(chunk.file_id, "0f8c1e6a");
    EXPECT_EQ(chunk.index, 0);
    // the relay copies these straight into the notify frame
    EXPECT_EQ(chunk.total, FileChunkTotal(4 * FILE_CHUNK_BYTES + 17));
}

TEST(ParseFileChunk, AcceptsAShortFinalChunk)
{
    Json::Value root = MakeChunk();
    root["index"] = root["total"].asInt() - 1;
    // 17 raw bytes encode to 24 base64 characters
    root["data"] = std::string(24, 'A');

    FileChunk chunk;
    EXPECT_TRUE(ParseFileChunk(root, chunk));
}

TEST(ParseFileChunk, RejectsAMissingOrWrongTypedField)
{
    for (const char* key : {"fileid", "filename", "filesize", "index", "total", "data"})
    {
        Json::Value root = MakeChunk();
        root.removeMember(key);
        FileChunk chunk;
        EXPECT_FALSE(ParseFileChunk(root, chunk)) << key;
    }

    for (const char* key : {"filesize", "index", "total"})
    {
        Json::Value root = MakeChunk();
        root[key] = "7";
        FileChunk chunk;
        EXPECT_FALSE(ParseFileChunk(root, chunk)) << key;
    }

    // a fractional size is not an int, but a whole-valued double still is to jsoncpp, so the
    // client sends the size as an int and this only guards the malformed case
    Json::Value root = MakeChunk();
    root["filesize"] = 4113.5;
    FileChunk chunk;
    EXPECT_FALSE(ParseFileChunk(root, chunk));
}

TEST(ParseFileChunk, RejectsBadSizesAndIndices)
{
    const auto rejects = [](const Json::Value& root) {
        FileChunk chunk;
        return !ParseFileChunk(root, chunk);
    };

    Json::Value root = MakeChunk();
    root["filesize"] = 0;
    EXPECT_TRUE(rejects(root));

    root = MakeChunk();
    root["filesize"] = -1;
    EXPECT_TRUE(rejects(root));

    root = MakeChunk();
    root["filesize"] = FILE_MAX_BYTES + 1;
    EXPECT_TRUE(rejects(root));

    // one byte over the limit still needs a total that matches, so report it as a size error
    root = MakeChunk();
    root["total"] = FileChunkTotal(FILE_MAX_BYTES + 1);
    EXPECT_TRUE(rejects(root));

    root = MakeChunk();
    root["total"] = 6;
    EXPECT_TRUE(rejects(root));

    root = MakeChunk();
    root["index"] = -1;
    EXPECT_TRUE(rejects(root));

    root = MakeChunk();
    root["index"] = root["total"].asInt();
    EXPECT_TRUE(rejects(root));
}

TEST(ParseFileChunk, RejectsBadIdsAndNames)
{
    Json::Value root = MakeChunk();
    root["fileid"] = "";
    FileChunk chunk;
    EXPECT_FALSE(ParseFileChunk(root, chunk));

    root = MakeChunk();
    root["fileid"] = std::string(65, 'a');
    EXPECT_FALSE(ParseFileChunk(root, chunk));

    root = MakeChunk();
    root["filename"] = "../../etc/passwd";
    EXPECT_FALSE(ParseFileChunk(root, chunk));
}

TEST(ParseFileChunk, RejectsDataThatIsntOneChunk)
{
    FileChunk chunk;

    // short chunk in a non-final position
    Json::Value root = MakeChunk();
    root["data"] = std::string(24, 'A');
    EXPECT_FALSE(ParseFileChunk(root, chunk));

    root = MakeChunk();
    root["data"] = "";
    EXPECT_FALSE(ParseFileChunk(root, chunk));

    root = MakeChunk();
    root["data"] = std::string(FILE_CHUNK_BASE64_LEN + 4, 'A');
    EXPECT_FALSE(ParseFileChunk(root, chunk));

    // length is a multiple of 4 but the alphabet is wrong
    root = MakeChunk();
    root["data"] = std::string(FILE_CHUNK_BASE64_LEN, '*');
    EXPECT_FALSE(ParseFileChunk(root, chunk));

    root = MakeChunk();
    root["data"] = std::string(FILE_CHUNK_BASE64_LEN - 1, 'A');
    EXPECT_FALSE(ParseFileChunk(root, chunk));
}

TEST(FileChunkFrame, WorstCaseStillFitsTheReceiveBuffer)
{
    // one full chunk with the longest name the server accepts, prettified by jsoncpp the way
    // the relay builds it: this is the largest body the server ever writes to a peer
    Json::Value root;
    root["error"] = 0;
    root["fromuid"] = 2147483647;
    root["touid"] = 2147483647;
    root["fileid"] = std::string(64, 'f');
    root["filename"] = std::string(FILE_MAX_NAME_BYTES, 'n');
    root["filesize"] = FILE_MAX_BYTES;
    root["index"] = 0;
    root["total"] = FileChunkTotal(FILE_MAX_BYTES);
    root["data"] = std::string(FILE_CHUNK_BASE64_LEN, 'A');

    const std::string body = root.toStyledString();
    EXPECT_LE(body.size(), MAX_LENGTH);
    EXPECT_TRUE(IsSendableBodyLen(body.size()));
}

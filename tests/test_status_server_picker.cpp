#include "ServerPicker.h"

#include <gtest/gtest.h>

#include <climits>
#include <map>
#include <string>
#include <vector>

using Names = std::vector<std::string>;
using Counts = std::map<std::string, std::string>;

TEST(ParseServerList, EmptyStringYieldsNoServers)
{
    EXPECT_TRUE(ParseServerList("").empty());
}

TEST(ParseServerList, SingleServer)
{
    EXPECT_EQ(ParseServerList("chatserver1"), Names{"chatserver1"});
}

TEST(ParseServerList, KeepsConfigOrder)
{
    EXPECT_EQ(ParseServerList("b,a,c"), (Names{"b", "a", "c"}));
}

TEST(ParseServerList, DropsEmptyNamesAndTrailingComma)
{
    EXPECT_EQ(ParseServerList(",a,,b,"), (Names{"a", "b"}));
    EXPECT_TRUE(ParseServerList(",,,").empty());
}

TEST(ParseServerList, TrimsWhitespaceIncludingCarriageReturn)
{
    EXPECT_EQ(ParseServerList(" a ,\tb\t, c\r"), (Names{"a", "b", "c"}));
    EXPECT_TRUE(ParseServerList("  ,\t, ").empty());
}

TEST(ParseServerList, DropsDuplicatesKeepingFirst)
{
    EXPECT_EQ(ParseServerList("a,b,a, b"), (Names{"a", "b"}));
}

TEST(ParseLoginCount, AcceptsNonNegativeIntegers)
{
    EXPECT_EQ(ParseLoginCount("0"), 0);
    EXPECT_EQ(ParseLoginCount("42"), 42);
    EXPECT_EQ(ParseLoginCount(std::to_string(INT_MAX)), INT_MAX);
}

TEST(ParseLoginCount, RejectsEmptyAndNonNumeric)
{
    EXPECT_FALSE(ParseLoginCount(""));
    EXPECT_FALSE(ParseLoginCount("abc"));
    EXPECT_FALSE(ParseLoginCount("12abc"));
    EXPECT_FALSE(ParseLoginCount(" 12"));
    EXPECT_FALSE(ParseLoginCount("12 "));
    EXPECT_FALSE(ParseLoginCount("1.5"));
    EXPECT_FALSE(ParseLoginCount("+1"));
}

TEST(ParseLoginCount, RejectsNegative)
{
    EXPECT_FALSE(ParseLoginCount("-1"));
    EXPECT_FALSE(ParseLoginCount(std::to_string(INT_MIN)));
}

TEST(ParseLoginCount, RejectsOverflow)
{
    EXPECT_FALSE(ParseLoginCount("2147483648"));
    EXPECT_FALSE(ParseLoginCount("99999999999999999999"));
}

TEST(PickServer, EmptyListPicksNothing)
{
    EXPECT_FALSE(PickServer({}, {}));
    EXPECT_FALSE(PickServer({}, Counts{{"a", "0"}}));
}

TEST(PickServer, AllCountsMissingPicksNothing)
{
    EXPECT_FALSE(PickServer({"a", "b"}, {}));
    EXPECT_FALSE(PickServer({"a", "b"}, Counts{{"a", ""}, {"b", ""}}));
}

TEST(PickServer, SingleServer)
{
    EXPECT_EQ(PickServer({"a"}, Counts{{"a", "7"}}), "a");
}

TEST(PickServer, PicksFewestLogins)
{
    EXPECT_EQ(PickServer({"a", "b", "c"}, Counts{{"a", "5"}, {"b", "2"}, {"c", "9"}}), "b");
}

TEST(PickServer, TiesGoToEarliestName)
{
    EXPECT_EQ(PickServer({"a", "b", "c"}, Counts{{"a", "3"}, {"b", "3"}, {"c", "3"}}), "a");
    EXPECT_EQ(PickServer({"c", "b", "a"}, Counts{{"a", "1"}, {"b", "1"}, {"c", "1"}}), "c");
}

TEST(PickServer, SkipsServersWithMissingOrInvalidCount)
{
    Counts counts{{"a", "abc"}, {"b", "-1"}, {"c", "99999999999"}, {"e", "4"}};
    EXPECT_EQ(PickServer({"a", "b", "c", "d", "e"}, counts), "e");
}

TEST(PickServer, AllCountsInvalidPicksNothing)
{
    EXPECT_FALSE(PickServer({"a", "b"}, Counts{{"a", "x"}, {"b", "-3"}}));
}

TEST(PickServer, IntMaxIsStillAvailable)
{
    auto max = std::to_string(INT_MAX);
    EXPECT_EQ(PickServer({"a"}, Counts{{"a", max}}), "a");
    EXPECT_EQ(PickServer({"a", "b"}, Counts{{"a", max}, {"b", max}}), "a");
    EXPECT_EQ(PickServer({"a", "b"}, Counts{{"a", max}, {"b", "0"}}), "b");
}

TEST(PickServer, IgnoresCountsForUnlistedServers)
{
    EXPECT_EQ(PickServer({"a"}, Counts{{"a", "10"}, {"z", "0"}}), "a");
}

// Unit tests for the ltg:regex(...) brick (modules/LuaRegex.h) — direct C++ calls.
// The std::regex port must keep the Lua-facing contract of the old boost::regex
// implementation: 1-based find positions, (result, count) gsub shape, and a real
// diagnostic (std::regex_error) on an invalid pattern.
//
// match()/gmatch() need a live sol::state (they return Lua values) — those paths are
// covered end-to-end through the Module tests (TestModule.cpp, LuaRegexThroughScript).

#include <modules/LuaRegex.h>

#include <gtest/gtest.h>

#include <regex>
#include <string>
#include <tuple>

TEST(LuaRegex, TestReportsAnyMatchAnywhere) {
    LuaRegex re("\\d+");
    EXPECT_TRUE(re.test("abc 123 def"));
    EXPECT_FALSE(re.test("abc def"));
}

TEST(LuaRegex, FindReturnsLuaStyleOneBasedInclusiveRange) {
    LuaRegex re("l+");
    const auto found = re.find("hello");
    ASSERT_TRUE(found.has_value());
    // "ll" spans bytes [2,4) 0-based → Lua-style [3,4] 1-based inclusive
    EXPECT_EQ(std::get<0>(*found), 3);
    EXPECT_EQ(std::get<1>(*found), 4);
}

TEST(LuaRegex, FindReturnsNilOnNoMatch) {
    LuaRegex re("z+");
    EXPECT_FALSE(re.find("hello").has_value());
}

TEST(LuaRegex, GsubReplacesAllAndCounts) {
    LuaRegex re("o");
    const auto result = re.gsub("foo boo", "X");
    EXPECT_EQ(std::get<0>(result), "fXX bXX");
    EXPECT_EQ(std::get<1>(result), 4);
}

TEST(LuaRegex, GsubSupportsCaptureGroupReferences) {
    LuaRegex re("(\\w+)=(\\d+)");
    const auto result = re.gsub("a=1 b=2", "$2:$1");
    EXPECT_EQ(std::get<0>(result), "1:a 2:b");
    EXPECT_EQ(std::get<1>(result), 2);
}

TEST(LuaRegex, GsubWithoutMatchReturnsInputAndZero) {
    LuaRegex re("zz");
    const auto result = re.gsub("abc", "X");
    EXPECT_EQ(std::get<0>(result), "abc");
    EXPECT_EQ(std::get<1>(result), 0);
}

TEST(LuaRegex, InvalidPatternThrowsRegexError) {
    // sol2's exception handler forwards e.what() to the Lua error path — the throw type
    // must stay a std::exception derivative for that to keep working.
    EXPECT_THROW(LuaRegex("("), std::regex_error);
}

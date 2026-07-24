// Unit tests for the pure text-analysis helpers behind the code editor
// (src/frontend/Components/CodeUtils.*). All columns are BYTE offsets —
// the ImCode coordinate system.

#include <frontend/Components/CodeUtils.h>

#include <gtest/gtest.h>

using ltg::code::computeSignatureArgIndex;
using ltg::code::extractCallTargetAt;
using ltg::code::extractTokenAt;
using ltg::code::filterByPrefix;
using ltg::code::isIdentChar;

///////////////////////////////////////////////////////////////////////////////
// extractTokenAt
///////////////////////////////////////////////////////////////////////////////

TEST(CodeUtilsExtractToken, FindsIdentifierFromAnyByteInsideIt) {
    const std::string line = "local myVar = 42";
    EXPECT_EQ(extractTokenAt(line, 6), "myVar");   // first byte
    EXPECT_EQ(extractTokenAt(line, 8), "myVar");   // middle byte
    EXPECT_EQ(extractTokenAt(line, 10), "myVar");  // last byte
}

TEST(CodeUtilsExtractToken, KeepsDigitsAndUnderscoresInsideIdentifiers) {
    EXPECT_EQ(extractTokenAt("foo_bar2 = x", 3), "foo_bar2");
    EXPECT_EQ(extractTokenAt("v2x", 1), "v2x");
}

TEST(CodeUtilsExtractToken, EmptyOnPunctuationAndWhitespace) {
    const std::string line = "a = b(c)";
    EXPECT_EQ(extractTokenAt(line, 1), "");  // space
    EXPECT_EQ(extractTokenAt(line, 2), "");  // '='
    EXPECT_EQ(extractTokenAt(line, 5), "");  // '('
}

TEST(CodeUtilsExtractToken, EmptyOnPureNumberLiterals) {
    EXPECT_EQ(extractTokenAt("x = 12345", 6), "");
}

TEST(CodeUtilsExtractToken, EmptyOutOfRange) {
    EXPECT_EQ(extractTokenAt("abc", -1), "");
    EXPECT_EQ(extractTokenAt("abc", 3), "");  // one past the end
    EXPECT_EQ(extractTokenAt("", 0), "");
}

TEST(CodeUtilsExtractToken, ByteColumnsOnTabbedLine) {
    // byte semantics: the tab is ONE byte — 'foo' starts at byte 1 regardless of tab width
    EXPECT_EQ(extractTokenAt("\tfoo", 1), "foo");
    EXPECT_EQ(extractTokenAt("\tfoo", 0), "");  // the tab itself
}

///////////////////////////////////////////////////////////////////////////////
// extractCallTargetAt — resolves `target?.functionName(`
///////////////////////////////////////////////////////////////////////////////

TEST(CodeUtilsCallTarget, MethodCallWithColonSeparator) {
    std::string target;
    std::string function;
    // "ltg:regex(" — the '(' is at byte 9
    ASSERT_TRUE(extractCallTargetAt("ltg:regex(", 9, target, function));
    EXPECT_EQ(target, "ltg");
    EXPECT_EQ(function, "regex");
}

TEST(CodeUtilsCallTarget, MemberCallWithDotSeparator) {
    std::string target;
    std::string function;
    ASSERT_TRUE(extractCallTargetAt("x = math.floor(", 14, target, function));
    EXPECT_EQ(target, "math");
    EXPECT_EQ(function, "floor");
}

TEST(CodeUtilsCallTarget, PlainFunctionCallHasEmptyTarget) {
    std::string target;
    std::string function;
    ASSERT_TRUE(extractCallTargetAt("foo(", 3, target, function));
    EXPECT_EQ(target, "");
    EXPECT_EQ(function, "foo");
}

TEST(CodeUtilsCallTarget, ChainedAccessKeepsOnlyTheLastSegmentAsTarget) {
    std::string target;
    std::string function;
    ASSERT_TRUE(extractCallTargetAt("a.b.c(", 5, target, function));
    EXPECT_EQ(target, "b");
    EXPECT_EQ(function, "c");
}

TEST(CodeUtilsCallTarget, RejectsWhitespaceBeforeParen) {
    std::string target;
    std::string function;
    EXPECT_FALSE(extractCallTargetAt("foo (", 4, target, function));
}

TEST(CodeUtilsCallTarget, RejectsNumberLiteralBeforeParen) {
    std::string target;
    std::string function;
    EXPECT_FALSE(extractCallTargetAt("42(", 2, target, function));
}

TEST(CodeUtilsCallTarget, RejectsParenAtLineStart) {
    std::string target;
    std::string function;
    EXPECT_FALSE(extractCallTargetAt("(", 0, target, function));
}

///////////////////////////////////////////////////////////////////////////////
// computeSignatureArgIndex — scan between the opening '(' and the caret
///////////////////////////////////////////////////////////////////////////////

TEST(CodeUtilsSignatureScan, ArgIndexFollowsTopLevelCommas) {
    //           0123456789
    const std::string line = "f(aa, bb, cc";
    int32_t argIndex = -1;
    ASSERT_TRUE(computeSignatureArgIndex(line, 2, 3, argIndex));  // inside first arg
    EXPECT_EQ(argIndex, 0);
    ASSERT_TRUE(computeSignatureArgIndex(line, 2, 7, argIndex));  // after first comma
    EXPECT_EQ(argIndex, 1);
    ASSERT_TRUE(computeSignatureArgIndex(line, 2, line.size(), argIndex));  // after second comma
    EXPECT_EQ(argIndex, 2);
}

TEST(CodeUtilsSignatureScan, NestedCallCommasAreIgnored) {
    //           0         1
    //           0123456789012345
    const std::string line = "f(g(a, b), c";
    int32_t argIndex = -1;
    ASSERT_TRUE(computeSignatureArgIndex(line, 2, line.size(), argIndex));
    EXPECT_EQ(argIndex, 1);  // only the comma AFTER g(...) counts at depth 0
}

TEST(CodeUtilsSignatureScan, CommasInsideStringLiteralsAreIgnored) {
    const std::string line = "f(\"a, b\", c";
    int32_t argIndex = -1;
    ASSERT_TRUE(computeSignatureArgIndex(line, 2, line.size(), argIndex));
    EXPECT_EQ(argIndex, 1);
}

TEST(CodeUtilsSignatureScan, EscapedQuoteDoesNotCloseTheString) {
    const std::string line = "f(\"a\\\", b\", c";  // f("a\", b", c
    int32_t argIndex = -1;
    ASSERT_TRUE(computeSignatureArgIndex(line, 2, line.size(), argIndex));
    EXPECT_EQ(argIndex, 1);
}

TEST(CodeUtilsSignatureScan, ClosedCallReportsFinished) {
    const std::string line = "f(a, b) + x";
    int32_t argIndex = -1;
    EXPECT_FALSE(computeSignatureArgIndex(line, 2, line.size(), argIndex));  // caret past the ')'
}

TEST(CodeUtilsSignatureScan, EmptySpanIsArgZero) {
    int32_t argIndex = -1;
    ASSERT_TRUE(computeSignatureArgIndex("f(", 2, 2, argIndex));
    EXPECT_EQ(argIndex, 0);
}

///////////////////////////////////////////////////////////////////////////////
// filterByPrefix
///////////////////////////////////////////////////////////////////////////////

TEST(CodeUtilsPrefixFilter, EmptyPrefixKeepsEverythingInOrder) {
    const std::vector<std::string> names{"addSignalValue", "addSignalTag", "regex"};
    const auto kept = filterByPrefix(names, "");
    ASSERT_EQ(kept.size(), 3u);
    EXPECT_EQ(kept[0], 0u);
    EXPECT_EQ(kept[2], 2u);
}

TEST(CodeUtilsPrefixFilter, PrefixIsCaseSensitive) {
    const std::vector<std::string> names{"addSignalValue", "AddSignalValue", "addSignalTag"};
    const auto kept = filterByPrefix(names, "addSignal");
    ASSERT_EQ(kept.size(), 2u);
    EXPECT_EQ(kept[0], 0u);
    EXPECT_EQ(kept[1], 2u);
}

TEST(CodeUtilsPrefixFilter, NoMatchGivesEmpty) {
    const std::vector<std::string> names{"foo", "bar"};
    EXPECT_TRUE(filterByPrefix(names, "zz").empty());
}

TEST(CodeUtilsPrefixFilter, PrefixLongerThanNameDoesNotMatch) {
    const std::vector<std::string> names{"ab"};
    EXPECT_TRUE(filterByPrefix(names, "abc").empty());
}

///////////////////////////////////////////////////////////////////////////////
// isIdentChar
///////////////////////////////////////////////////////////////////////////////

TEST(CodeUtilsIdentChar, Classification) {
    EXPECT_TRUE(isIdentChar('a'));
    EXPECT_TRUE(isIdentChar('Z'));
    EXPECT_TRUE(isIdentChar('0'));
    EXPECT_TRUE(isIdentChar('_'));
    EXPECT_FALSE(isIdentChar(' '));
    EXPECT_FALSE(isIdentChar('.'));
    EXPECT_FALSE(isIdentChar(':'));
    EXPECT_FALSE(isIdentChar('('));
}

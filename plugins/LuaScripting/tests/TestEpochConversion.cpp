// Thorough tests of the two epoch conversion functions exposed to Lua as
// ltg:stringToEpoch(...) / ltg:epochToString(...) (modules/LuaDatasModel.cpp).
//
// Covered dialects:
//  - Joda-style      : "yyyy-MM-dd HH:mm:ss,SSS" (the default pattern) — MM before the
//                      hour token is a MONTH
//  - colloquial time : "HH:MM:SS.MS" — MM after the hour token is MINUTES, trailing MS
//                      is a milliseconds fraction. This is the log-file dialect that used
//                      to throw ("19:05:39.146503" was the original repro).
//
// Determinism notes: stringToEpoch interprets the input as UTC via a mktime/gmtime
// compensation that depends on the host timezone database, so absolute values are
// asserted through epochToString round-trips (gmtime — pure UTC) or through
// DIFFERENCES between two parses, both of which cancel the timezone out.

#include <modules/LuaDatasModel.h>

#include <gtest/gtest.h>

#include <cmath>
#include <stdexcept>
#include <string>

namespace {

double parse(const std::string& aText, double aHourOffset, const std::string& aPattern) {
    LuaDatasModel model;
    return model.luaModuleStringToEpochWithPattern(aText, aHourOffset, aPattern);
}

std::string format(double aEpoch, double aHourOffset, const std::string& aPattern) {
    LuaDatasModel model;
    return model.luaModuleEpochToStringWithPattern(aEpoch, aHourOffset, aPattern);
}

double fractionalPart(double aValue) {
    return aValue - std::floor(aValue);
}

}  // namespace

///////////////////////////////////////////////////////////////////////////////
// stringToEpoch — colloquial time patterns ("HH:MM:SS.MS")
///////////////////////////////////////////////////////////////////////////////

TEST(EpochFromString, OriginalRepro_TimeWithMicrosecondsAndMsPattern) {
    // the exact case that used to throw: 6 fractional digits against a .MS pattern.
    // the INPUT digit count wins — 146503 is read as microseconds.
    const double epoch = parse("19:05:39.146503", 0.0, "HH:MM:SS.MS");
    EXPECT_NEAR(fractionalPart(epoch), 0.146503, 1e-9);
}

TEST(EpochFromString, TimeWithMillisecondsAndMsPattern) {
    const double epoch = parse("19:05:39.146", 0.0, "HH:MM:SS.MS");
    EXPECT_NEAR(fractionalPart(epoch), 0.146, 1e-9);
}

TEST(EpochFromString, MmAfterHourMeansMinutes) {
    // one minute apart parses as 60 seconds apart — MM in a time context is NOT a month
    const double t0 = parse("10:20:30", 0.0, "HH:MM:SS");
    const double t1 = parse("10:21:30", 0.0, "HH:MM:SS");
    EXPECT_DOUBLE_EQ(t1 - t0, 60.0);
}

TEST(EpochFromString, TrailingSecondsAreNotMistakenForAFraction) {
    // "HH:MM:SS" ends with an S run but ':' is not a fraction delimiter — SS is seconds
    const double t0 = parse("19:05:39", 0.0, "HH:MM:SS");
    const double t1 = parse("19:05:40", 0.0, "HH:MM:SS");
    EXPECT_DOUBLE_EQ(t1 - t0, 1.0);
    EXPECT_NEAR(fractionalPart(t0), 0.0, 1e-9);
}

TEST(EpochFromString, TimeOnlyPatternIsTheTimeOfDayInSeconds) {
    // time-only patterns anchor on the epoch day itself: the value IS the time of day.
    // (pure-arithmetic conversion — the old mktime path returned -1 on Windows here)
    EXPECT_DOUBLE_EQ(parse("00:00:01", 0.0, "HH:MM:SS"), 1.0);
    EXPECT_DOUBLE_EQ(parse("19:05:39", 0.0, "HH:MM:SS"), 19.0 * 3600.0 + 5.0 * 60.0 + 39.0);
}

///////////////////////////////////////////////////////////////////////////////
// stringToEpoch — Joda patterns (the default "yyyy-MM-dd HH:mm:ss,SSS")
///////////////////////////////////////////////////////////////////////////////

TEST(EpochFromString, DefaultJodaPatternParsesMilliseconds) {
    // 1e-6 tolerance: a double around 1.7e9 s (year 2024) has a ~2e-7 s quantum, so
    // millisecond fractions cannot be asserted tighter than the microsecond
    LuaDatasModel model;
    const double t0 = model.luaModuleStringToEpoch("2024-01-15 08:30:45,123", 0.0);
    const double t1 = model.luaModuleStringToEpoch("2024-01-15 08:30:45,124", 0.0);
    EXPECT_NEAR(fractionalPart(t0), 0.123, 1e-6);
    EXPECT_NEAR(t1 - t0, 0.001, 1e-6);
}

TEST(EpochFromString, MmBeforeHourMeansMonth) {
    // same wall time one month apart: January (31 days) to February
    const double january = parse("2024-01-15 10:00:00", 0.0, "yyyy-MM-dd HH:mm:ss");
    const double february = parse("2024-02-15 10:00:00", 0.0, "yyyy-MM-dd HH:mm:ss");
    EXPECT_DOUBLE_EQ(february - january, 31.0 * 86400.0);
}

TEST(EpochFromString, DayFieldMovesByWholeDays) {
    const double day0 = parse("2024-01-15 10:00:00", 0.0, "yyyy-MM-dd HH:mm:ss");
    const double day1 = parse("2024-01-16 10:00:00", 0.0, "yyyy-MM-dd HH:mm:ss");
    EXPECT_DOUBLE_EQ(day1 - day0, 86400.0);
}

TEST(EpochFromString, InputFractionDigitCountWinsOverThePattern) {
    // pattern says 3 'S' but the log line carries 1 or 6 digits — scale by what is there
    const double oneDigit = parse("12:00:00.5", 0.0, "HH:mm:ss.SSS");
    EXPECT_NEAR(fractionalPart(oneDigit), 0.5, 1e-9);
    const double sixDigits = parse("12:00:00.146503", 0.0, "HH:mm:ss.SSS");
    EXPECT_NEAR(fractionalPart(sixDigits), 0.146503, 1e-9);
}

TEST(EpochFromString, HourOffsetShiftsTheResult) {
    const double base = parse("10:00:00", 0.0, "HH:MM:SS");
    const double plusTwo = parse("10:00:00", 2.0, "HH:MM:SS");
    const double halfHour = parse("10:00:00", 5.5, "HH:MM:SS");  // +5.5h timezones exist
    EXPECT_DOUBLE_EQ(plusTwo - base, 7200.0);
    EXPECT_DOUBLE_EQ(halfHour - base, 5.5 * 3600.0);
}

///////////////////////////////////////////////////////////////////////////////
// stringToEpoch — error paths
///////////////////////////////////////////////////////////////////////////////

TEST(EpochFromString, GarbageInputThrows) {
    EXPECT_THROW(parse("garbage", 0.0, "HH:MM:SS"), std::invalid_argument);
}

TEST(EpochFromString, MissingFractionWhenPatternRequiresItThrows) {
    EXPECT_THROW(parse("19:05:39", 0.0, "HH:MM:SS.MS"), std::invalid_argument);
}

TEST(EpochFromString, WrongFractionDelimiterThrows) {
    EXPECT_THROW(parse("19:05:39,146", 0.0, "HH:MM:SS.MS"), std::invalid_argument);
}

TEST(EpochFromString, EmptyPatternThrows) {
    EXPECT_THROW(parse("19:05:39", 0.0, ""), std::invalid_argument);
}

///////////////////////////////////////////////////////////////////////////////
// epochToString — pure UTC formatting (deterministic absolute values)
///////////////////////////////////////////////////////////////////////////////

TEST(EpochToString, TimeOnlyWithMillisecondsFraction) {
    EXPECT_EQ(format(10.146, 0.0, "HH:MM:SS.MS"), "00:00:10.146");
}

TEST(EpochToString, FractionIsRoundedNotTruncated) {
    // 0.5 s is not exactly representable through (x - floor(x)) * 1e6 without rounding —
    // the old truncating cast rendered ".499"
    EXPECT_EQ(format(1.5, 0.0, "HH:MM:SS.MS"), "00:00:01.500");
    EXPECT_EQ(format(2.146503, 0.0, "HH:MM:SS.MS"), "00:00:02.146");
}

TEST(EpochToString, DefaultJodaPattern) {
    LuaDatasModel model;
    // 1970-01-02 00:01:02.500 UTC
    EXPECT_EQ(model.luaModuleEpochToString(86400.0 + 62.5, 0.0), "1970-01-02 00:01:02,500");
}

TEST(EpochToString, HourOffsetIsNormalizedAcrossMidnight) {
    // 23:00:10 + 2h = 01:00:10 the next day — the old tm_hour += path printed "25:00:10"
    EXPECT_EQ(format(23.0 * 3600.0 + 10.0, 2.0, "HH:MM:SS"), "01:00:10");
}

TEST(EpochToString, FractionalHourOffset) {
    EXPECT_EQ(format(0.0, 5.5, "HH:MM:SS"), "05:30:00");
}

///////////////////////////////////////////////////////////////////////////////
// round-trips (timezone-independent: parse treats input as UTC, format emits UTC)
///////////////////////////////////////////////////////////////////////////////

TEST(EpochRoundTrip, TimeWithMsPattern) {
    const std::string text = "19:05:39.146";
    EXPECT_EQ(format(parse(text, 0.0, "HH:MM:SS.MS"), 0.0, "HH:MM:SS.MS"), text);
}

TEST(EpochRoundTrip, DefaultJodaPattern) {
    LuaDatasModel model;
    const std::string text = "2024-01-15 08:30:45,123";
    const double epoch = model.luaModuleStringToEpoch(text, 0.0);
    EXPECT_EQ(model.luaModuleEpochToString(epoch, 0.0), text);
}

TEST(EpochRoundTrip, MicrosecondPrecisionSurvivesTheEpochDouble) {
    // format truncates to the pattern's 3 digits, but the epoch itself keeps the µs
    const double epoch = parse("19:05:39.146503", 0.0, "HH:MM:SS.MS");
    EXPECT_EQ(format(epoch, 0.0, "HH:MM:SS.MS"), "19:05:39.146");
    EXPECT_NEAR(fractionalPart(epoch), 0.146503, 1e-9);
}

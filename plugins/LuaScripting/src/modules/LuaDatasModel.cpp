#include <modules/LuaDatasModel.h>
#include <ezlibs/ezTools.hpp>
#include <ezlibs/ezLog.hpp>
#include <cmath>  // floor / llround — fractional-seconds + hour-offset math

LuaDatasModelPtr LuaDatasModel::create(Ltg::IDatasModelWeak vIDatasModel) {
    auto res = std::make_shared<LuaDatasModel>();
    res->m_DatasModel = vIDatasModel;
    if (vIDatasModel.expired()) {
        res.reset();
    }
    return res;
}

void LuaDatasModel::luaModuleLogInfo(const std::string& vKey) {
    if (!vKey.empty()) {
        LogVarLightInfo("%s", vKey.c_str());
    }
}

void LuaDatasModel::luaModuleLogWarning(const std::string& vKey) {
    if (!vKey.empty()) {
        LogVarLightWarning("%s", vKey.c_str());
    }
}

void LuaDatasModel::luaModuleLogError(const std::string& vKey) {
    if (!vKey.empty()) {
        LogVarLightError("%s", vKey.c_str());
    }
}

void LuaDatasModel::luaModuleLogDebug(const std::string& vKey) {
    if (!vKey.empty()) {
        LogVarDebugInfo("%s", vKey.c_str());
    }
}

void LuaDatasModel::setRowIndex(int32_t vRowIndex) {
    m_RowIndex = vRowIndex;
}

double LuaDatasModel::luaModuleGetRowIndex() {
    return static_cast<double>(m_RowIndex);
}

void LuaDatasModel::setRowCount(int32_t vRowCount) {
    m_RowCount = vRowCount;
}

double LuaDatasModel::luaModuleGetRowCount() {
    return static_cast<double>(m_RowCount);
}

namespace {

// Default Joda-style pattern when the 2-arg form is used — backward-compatible with the
// previously hardcoded "YYYY-MM-DD HH:MM:SS,MS" / ".MS" inputs.
constexpr const char* sDefaultJodaPattern = "yyyy-MM-dd HH:mm:ss,SSS";

// Translate a Joda-style pattern (yyyy / MM / dd / HH / mm / ss + optional trailing 'S' run)
// into a strftime format string for std::get_time / std::put_time, plus the fractional-seconds
// delimiter and digit count. The 'S' run, if present, MUST be at the end of the pattern and
// MUST be preceded by exactly one literal delimiter character.
// Translate a date pattern into a strftime format + an optional fractional-seconds part.
// Two dialects are accepted:
//  - Joda-style   : yyyy / yy / MM (month) / dd / HH / hh / mm (minutes) / ss
//                   + trailing '.'/','-delimited 'S' run for the fraction ("HH:mm:ss.SSS")
//  - colloquial   : uppercase MM / SS in a TIME context ("HH:MM:SS.MS") — MM after an hour
//                   token means MINUTES, SS means SECONDS, and a trailing MS means a
//                   milliseconds fraction. This is what log files commonly use.
// Disambiguation is positional: an "MM" seen AFTER an hour token (HH/hh) is minutes,
// before it is a month ("yyyy-MM-dd HH:mm:ss" keeps its meaning). The fraction marker is
// only recognized after a '.' or ',' delimiter, so a pattern simply ENDING with seconds
// ("HH:MM:SS") is not mistaken for a fraction.
// aoHasDateTokens reports whether the pattern carries any DATE field (year/month/day) —
// time-only patterns get a fixed fallback date at parse time (see stringToEpoch).
void translateJodaPattern(const std::string& vPattern, std::string& aoStrftimeFmt, char& aoFracDelim, int32_t& aoFracDigits, bool& aoHasDateTokens) {
    if (vPattern.empty()) {
        throw std::invalid_argument("pattern: empty");
    }
    auto isFracDelim = [](char aChar) { return aChar == '.' || aChar == ','; };

    std::string dateOnly = vPattern;
    aoFracDelim = 0;
    aoFracDigits = 0;
    // trailing "MS" (milliseconds marker) preceded by '.' or ','
    if (vPattern.size() >= 3 && vPattern.compare(vPattern.size() - 2, 2, "MS") == 0 && isFracDelim(vPattern[vPattern.size() - 3])) {
        aoFracDelim = vPattern[vPattern.size() - 3];
        aoFracDigits = 3;
        dateOnly = vPattern.substr(0, vPattern.size() - 3);
    } else {
        // trailing 'S' run (Joda fraction) preceded by '.' or ','
        std::size_t fracStart = vPattern.size();
        while (fracStart > 0 && vPattern[fracStart - 1] == 'S') {
            --fracStart;
        }
        if (fracStart < vPattern.size() && fracStart > 0 && isFracDelim(vPattern[fracStart - 1])) {
            aoFracDelim = vPattern[fracStart - 1];
            aoFracDigits = static_cast<int32_t>(vPattern.size() - fracStart);
            dateOnly = vPattern.substr(0, fracStart - 1);
        }
    }

    // single left-to-right pass so the MM/SS disambiguation can look at what came before
    aoStrftimeFmt.clear();
    aoStrftimeFmt.reserve(dateOnly.size() + 8);
    aoHasDateTokens = false;
    bool hourSeen = false;
    std::size_t i = 0;
    auto match = [&](const char* aToken, std::size_t aLen) { return dateOnly.compare(i, aLen, aToken) == 0; };
    while (i < dateOnly.size()) {
        if (match("yyyy", 4)) {
            aoStrftimeFmt += "%Y";
            aoHasDateTokens = true;
            i += 4;
        } else if (match("yy", 2)) {
            aoStrftimeFmt += "%y";
            aoHasDateTokens = true;
            i += 2;
        } else if (match("dd", 2)) {
            aoStrftimeFmt += "%d";
            aoHasDateTokens = true;
            i += 2;
        } else if (match("HH", 2)) {
            aoStrftimeFmt += "%H";
            hourSeen = true;
            i += 2;
        } else if (match("hh", 2)) {
            aoStrftimeFmt += "%I";
            hourSeen = true;
            i += 2;
        } else if (match("mm", 2)) {
            aoStrftimeFmt += "%M";
            i += 2;
        } else if (match("ss", 2)) {
            aoStrftimeFmt += "%S";
            i += 2;
        } else if (match("MM", 2)) {
            // month before the hour ("yyyy-MM-dd..."), minutes after it ("HH:MM:SS")
            if (hourSeen) {
                aoStrftimeFmt += "%M";
            } else {
                aoStrftimeFmt += "%m";
                aoHasDateTokens = true;
            }
            i += 2;
        } else if (match("SS", 2)) {
            aoStrftimeFmt += "%S";
            i += 2;
        } else {
            aoStrftimeFmt += dateOnly[i];
            ++i;
        }
    }
}

// Days between 1970-01-01 and the given civil date (proleptic Gregorian) — Howard Hinnant's
// algorithm. Pure integer arithmetic: no mktime, no timezone database, no DST, identical on
// every platform (and valid before 1970, where Windows' mktime returns -1).
int64_t daysFromCivil(int64_t vYear, int64_t vMonth, int64_t vDay) {
    vYear -= vMonth <= 2;
    const int64_t era = (vYear >= 0 ? vYear : vYear - 399) / 400;
    const int64_t yearOfEra = vYear - era * 400;                                        // [0, 399]
    const int64_t dayOfYear = (153 * (vMonth + (vMonth > 2 ? -3 : 9)) + 2) / 5 + vDay - 1;  // [0, 365]
    const int64_t dayOfEra = yearOfEra * 365 + yearOfEra / 4 - yearOfEra / 100 + dayOfYear;  // [0, 146096]
    return era * 146097 + dayOfEra - 719468;
}

// Convert a fractional-seconds integer with `vDigits` digits into microseconds (6 digits).
int64_t scaleFractionToMicros(int64_t vValue, int32_t vDigits) {
    if (vDigits == 6) {
        return vValue;
    }
    int64_t factor = 1;
    if (vDigits < 6) {
        for (int32_t i = 0; i < 6 - vDigits; ++i) {
            factor *= 10;
        }
        return vValue * factor;
    }
    for (int32_t i = 0; i < vDigits - 6; ++i) {
        factor *= 10;
    }
    return vValue / factor;
}

// Convert microseconds (6 digits) into a fractional-seconds integer with `vDigits` digits.
int64_t scaleMicrosToFraction(int64_t vMicros, int32_t vDigits) {
    if (vDigits == 6) {
        return vMicros;
    }
    int64_t factor = 1;
    if (vDigits < 6) {
        for (int32_t i = 0; i < 6 - vDigits; ++i) {
            factor *= 10;
        }
        return vMicros / factor;
    }
    for (int32_t i = 0; i < vDigits - 6; ++i) {
        factor *= 10;
    }
    return vMicros * factor;
}

}  // namespace

double LuaDatasModel::luaModuleStringToEpoch(const std::string& vDateTime, double vHourOffset) {
    return luaModuleStringToEpochWithPattern(vDateTime, vHourOffset, sDefaultJodaPattern);
}

double LuaDatasModel::luaModuleStringToEpochWithPattern(const std::string& vDateTime, double vHourOffset, const std::string& vPattern) {
    std::string strftimeFmt;
    char fracDelim = 0;
    int32_t fracDigits = 0;
    bool hasDateTokens = false;
    translateJodaPattern(vPattern, strftimeFmt, fracDelim, fracDigits, hasDateTokens);

    struct tm timeStruct = {};
    std::istringstream dateStream(vDateTime);
    dateStream >> std::get_time(&timeStruct, strftimeFmt.c_str());
    if (dateStream.fail()) {
        throw std::invalid_argument("Invalid date format");
    }
    if (!hasDateTokens) {
        // time-only pattern ("HH:MM:SS.MS", ...): anchor on the epoch day itself, so the
        // returned value is literally the time of day in seconds. Only differences between
        // such epochs are meaningful anyway.
        timeStruct.tm_year = 70;  // 1970
        timeStruct.tm_mon = 0;
        timeStruct.tm_mday = 1;
    }

    int64_t microseconds = 0;
    if (fracDigits > 0) {
        // the digit count of the INPUT wins over the pattern's: log files mix precisions
        // freely ("19:05:39.146503" is microseconds even when the pattern says .MS), so
        // read the whole digit run and scale by what was actually there.
        char delimiter = 0;
        dateStream >> delimiter;
        if (dateStream.fail() || delimiter != fracDelim) {
            throw std::invalid_argument("Invalid fractional seconds");
        }
        std::string digitRun;
        while (dateStream.good()) {
            const int nextChar = dateStream.peek();
            if (nextChar < '0' || nextChar > '9') {
                break;
            }
            digitRun.push_back(static_cast<char>(dateStream.get()));
        }
        if (digitRun.empty()) {
            throw std::invalid_argument("Invalid fractional seconds");
        }
        if (digitRun.size() > 9U) {
            digitRun.resize(9U);  // nanosecond precision is more than enough — drop the tail
        }
        microseconds = scaleFractionToMicros(std::stoll(digitRun), static_cast<int32_t>(digitRun.size()));
    }

    // the input string IS UTC: pure arithmetic civil-date conversion, no mktime. The old
    // mktime + gmtime/localtime compensation depended on the HOST timezone database and
    // the forced tm_isdst — it drifted by one hour in any DST-bearing timezone.
    int64_t epochSeconds = daysFromCivil(timeStruct.tm_year + 1900, timeStruct.tm_mon + 1, timeStruct.tm_mday) * 86400  //
        + timeStruct.tm_hour * 3600 + timeStruct.tm_min * 60 + timeStruct.tm_sec;
    // hour offset applied on the RESULT — fractional offsets (+5.5h timezones) stay exact
    epochSeconds += static_cast<int64_t>(std::llround(vHourOffset * 3600.0));

    return static_cast<double>(epochSeconds) + static_cast<double>(microseconds) / 1000000.0;
}

std::string LuaDatasModel::luaModuleEpochToString(double vEpochTime, double vHourOffset) {
    return luaModuleEpochToStringWithPattern(vEpochTime, vHourOffset, sDefaultJodaPattern);
}

std::string LuaDatasModel::luaModuleEpochToStringWithPattern(double vEpochTime, double vHourOffset, const std::string& vPattern) {
    std::string strftimeFmt;
    char fracDelim = 0;
    int32_t fracDigits = 0;
    bool hasDateTokens = false;
    translateJodaPattern(vPattern, strftimeFmt, fracDelim, fracDigits, hasDateTokens);

    // floor + rounded remainder: a plain cast truncates and (x - seconds) * 1e6 lands one
    // microsecond short for most decimal fractions (0.146503 -> 146502.99...), which used
    // to shave the last digit off the formatted fraction. Carry on a full-second round-up.
    std::time_t seconds = static_cast<std::time_t>(std::floor(vEpochTime));
    int64_t microseconds = static_cast<int64_t>(std::llround((vEpochTime - static_cast<double>(seconds)) * 1000000.0));
    if (microseconds >= 1000000) {
        seconds += 1;
        microseconds -= 1000000;
    }
    // hour offset on the epoch itself: gmtime then formats normalized fields (the old
    // `tm_hour += offset` path emitted raw un-normalized hours like "25" via put_time)
    seconds += static_cast<std::time_t>(std::llround(vHourOffset * 3600.0));
    struct tm* timeStruct = std::gmtime(&seconds);
    if (!timeStruct) {
        throw std::runtime_error("Failed to convert epoch time to struct tm");
    }

    std::ostringstream dateStream;
    dateStream << std::put_time(timeStruct, strftimeFmt.c_str());
    if (fracDigits > 0) {
        const int64_t fracValue = scaleMicrosToFraction(microseconds, fracDigits);
        dateStream << fracDelim << std::setfill('0') << std::setw(fracDigits) << fracValue;
    }
    return dateStream.str();
}

void LuaDatasModel::luaModuleAddSignalValue(const std::string& vCategory, const std::string& vName, double vEpoch, double vValue) {
    auto ptr = m_DatasModel.lock();
    if (ptr != nullptr) {
        ptr->addSignalValue(vCategory, vName, vEpoch, vValue, {});
    }
}

void LuaDatasModel::luaModuleAddSignalValueWithDesc(const std::string& vCategory, const std::string& vName, double vEpoch, double vValue, const std::string& vDesc) {
    auto ptr = m_DatasModel.lock();
    if (ptr != nullptr) {
        ptr->addSignalValue(vCategory, vName, vEpoch, vValue, vDesc);
    }
}

void LuaDatasModel::luaModuleAddSignalTag(double vEpoch, double r, double g, double b, double a, const std::string& vName, const std::string& vHelp) {
    auto ptr = m_DatasModel.lock();
    if (ptr != nullptr) {
        ptr->addSignalTag(vEpoch, r, g, b, a, vName, vHelp);
    }
}

void LuaDatasModel::luaModuleAddSignalStatus(const std::string& vCategory, const std::string& vName, double vEpoch, const std::string& vStatus) {
    auto ptr = m_DatasModel.lock();
    if (ptr != nullptr) {
        ptr->addSignalStatus(vCategory, vName, vEpoch, vStatus);
    }
}

void LuaDatasModel::luaModuleAddSignalStartZone(const std::string& vCategory, const std::string& vName, double vEpoch, const std::string& vStartMsg) {
    auto ptr = m_DatasModel.lock();
    if (ptr != nullptr) {
        ptr->addSignalStartZone(vCategory, vName, vEpoch, vStartMsg);
    }
}

void LuaDatasModel::luaModuleAddSignalEndZone(const std::string& vCategory, const std::string& vName, double vEpoch, const std::string& vEndMsg) {
    auto ptr = m_DatasModel.lock();
    if (ptr != nullptr) {
        ptr->addSignalEndZone(vCategory, vName, vEpoch, vEndMsg);
    }
}

LuaRegex LuaDatasModel::luaModuleRegex(const std::string& vPattern) const {
    // std::regex constructor throws std::regex_error (derives from std::runtime_error) on
    // an invalid pattern. sol2's exception_handler catches it and pushes e.what() as the Lua
    // error message, so the user sees the real regex diagnostic at the call site.
    return LuaRegex(vPattern);
}

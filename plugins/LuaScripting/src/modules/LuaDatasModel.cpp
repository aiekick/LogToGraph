#include <modules/LuaDatasModel.h>
#include <ezlibs/ezTools.hpp>
#include <ezlibs/ezLog.hpp>

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
void translateJodaPattern(const std::string& vPattern, std::string& aoStrftimeFmt, char& aoFracDelim, int32_t& aoFracDigits) {
    if (vPattern.empty()) {
        throw std::invalid_argument("pattern: empty");
    }
    // walk back from the end to count the trailing 'S' run (Joda fractional seconds marker)
    std::size_t fracStart = vPattern.size();
    while (fracStart > 0 && vPattern[fracStart - 1] == 'S') {
        --fracStart;
    }
    std::string dateOnly;
    if (fracStart == vPattern.size()) {
        dateOnly = vPattern;
        aoFracDelim = 0;
        aoFracDigits = 0;
    } else if (fracStart == 0) {
        throw std::invalid_argument("pattern: fractional 'S' must be preceded by a delimiter");
    } else {
        dateOnly = vPattern.substr(0, fracStart - 1);
        aoFracDelim = vPattern[fracStart - 1];
        aoFracDigits = static_cast<int32_t>(vPattern.size() - fracStart);
    }
    aoStrftimeFmt = dateOnly;
    // longer tokens first so 'yyyy' is consumed before 'yy' tries to match
    // clang-format off
    const std::pair<std::string, std::string> tokens[] = {
        {"yyyy", "%Y"}, {"yy", "%y"},
        {"MM",   "%m"},
        {"dd",   "%d"},
        {"HH",   "%H"}, {"hh", "%I"},
        {"mm",   "%M"},
        {"ss",   "%S"},
    };
    // clang-format on
    for (const auto& tk : tokens) {
        std::size_t pos = 0;
        while ((pos = aoStrftimeFmt.find(tk.first, pos)) != std::string::npos) {
            aoStrftimeFmt.replace(pos, tk.first.size(), tk.second);
            pos += tk.second.size();
        }
    }
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
    translateJodaPattern(vPattern, strftimeFmt, fracDelim, fracDigits);

    struct tm timeStruct = {};
    std::istringstream dateStream(vDateTime);
    dateStream >> std::get_time(&timeStruct, strftimeFmt.c_str());
    if (dateStream.fail()) {
        throw std::invalid_argument("Invalid date format");
    }
    timeStruct.tm_hour += static_cast<int32_t>(vHourOffset);
    timeStruct.tm_isdst = 1;

    int64_t fracValue = 0;
    if (fracDigits > 0) {
        char delimiter = 0;
        dateStream >> delimiter >> fracValue;
        if (dateStream.fail() || delimiter != fracDelim) {
            throw std::invalid_argument("Invalid fractional seconds");
        }
    }

    std::time_t epochSeconds = std::mktime(&timeStruct);
    if (epochSeconds == -1) {
        throw std::runtime_error("Failed to convert to epoch time");
    }
    epochSeconds -= static_cast<time_t>(std::difftime(std::mktime(std::gmtime(&epochSeconds)), std::mktime(std::localtime(&epochSeconds))));

    const int64_t microseconds = (fracDigits > 0) ? scaleFractionToMicros(fracValue, fracDigits) : 0;
    return static_cast<double>(epochSeconds) + static_cast<double>(microseconds) / 1000000.0;
}

std::string LuaDatasModel::luaModuleEpochToString(double vEpochTime, double vHourOffset) {
    return luaModuleEpochToStringWithPattern(vEpochTime, vHourOffset, sDefaultJodaPattern);
}

std::string LuaDatasModel::luaModuleEpochToStringWithPattern(double vEpochTime, double vHourOffset, const std::string& vPattern) {
    std::string strftimeFmt;
    char fracDelim = 0;
    int32_t fracDigits = 0;
    translateJodaPattern(vPattern, strftimeFmt, fracDelim, fracDigits);

    std::time_t seconds = static_cast<std::time_t>(vEpochTime);
    const int64_t microseconds = static_cast<int64_t>((vEpochTime - seconds) * 1000000.0);
    struct tm* timeStruct = std::gmtime(&seconds);
    if (!timeStruct) {
        throw std::runtime_error("Failed to convert epoch time to struct tm");
    }
    timeStruct->tm_hour += static_cast<int32_t>(vHourOffset);
    timeStruct->tm_isdst = 1;

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
    // boost::regex constructor throws boost::regex_error (derives from std::runtime_error) on
    // an invalid pattern. sol2's exception_handler catches it and pushes e.what() as the Lua
    // error message, so the user sees the real boost diagnostic at the call site.
    return LuaRegex(vPattern);
}

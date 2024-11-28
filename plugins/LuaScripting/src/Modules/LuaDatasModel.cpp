#include <Modules/LuaDatasModel.h>
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

double LuaDatasModel::luaModuleStringToEpoch(const std::string& vDateTime, double vHourOffset) {
    struct tm timeStruct = {};
    int microseconds = 0;
    std::istringstream dateStream(vDateTime);
    char delimiter;
    dateStream >> std::get_time(&timeStruct, "%Y-%m-%d %H:%M:%S");
    if (dateStream.fail()) {
        throw std::invalid_argument("Invalid date format");
    }
    timeStruct.tm_hour += static_cast<int32_t>(vHourOffset);
    timeStruct.tm_isdst = 1;
    dateStream >> delimiter >> microseconds;
    std::time_t epochSeconds = std::mktime(&timeStruct);
    if (epochSeconds == -1) {
        throw std::runtime_error("Failed to convert to epoch time");
    }
    epochSeconds -= std::difftime(std::mktime(std::gmtime(&epochSeconds)), std::mktime(std::localtime(&epochSeconds)));
    return static_cast<double>(epochSeconds) + static_cast<double>(microseconds) / 1000000.0;
}

std::string LuaDatasModel::luaModuleEpochToString(double vEpochTime, double vHourOffset) {
    std::time_t seconds = static_cast<std::time_t>(vEpochTime);
    int microseconds = static_cast<int>((vEpochTime - seconds) * 1000000.0);
    struct tm* timeStruct = std::gmtime(&seconds);
    if (!timeStruct) {
        throw std::runtime_error("Failed to convert epoch time to struct tm");
    }
    timeStruct->tm_hour += static_cast<int32_t>(vHourOffset);
    timeStruct->tm_isdst = 1;
    std::ostringstream dateStream;
    dateStream << std::put_time(timeStruct, "%Y-%m-%d %H:%M:%S");
    dateStream << '.' << std::setfill('0') << std::setw(6) << microseconds;
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

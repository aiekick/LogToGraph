#pragma once

#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>

#include <apis/LtgPluginApi.h>
#include <memory>
#include <cstdint>
#include <string>

class LuaDatasModel;
typedef std::shared_ptr<LuaDatasModel> LuaDatasModelPtr;
typedef std::weak_ptr<LuaDatasModel> LuaDatasModelWeak;

class LuaDatasModel {
public:
    static LuaDatasModelPtr create(Ltg::IDatasModelWeak vIDatasModel);

private:
    Ltg::IDatasModelWeak m_DatasModel;

public:
    void init();
    void setScriptDescription(const std::string& vKey);
    void setRowBufferName(const std::string& vKey);
    void setFunctionForEachRow(const std::string& vKey);
    void setFunctionForEndFile(const std::string& vKey);

    void logInfo(const std::string& vKey);
    void logWarning(const std::string& vKey);
    void logError(const std::string& vKey);
    void logDebug(const std::string& vKey);

    int32_t getRowIndex();
    int32_t getRowCount();

    
    // todo : to test
    double stringToEpoch(const std::string& vDateTime, double vHourOffset);

    // todo : to test
    std::string epochToString(double vEpochTime, double vHourOffset);

    void addSignalTag(double vEpoch,
                      double r,
                      double g,
                      double b,
                      double a,
                      const std::string& vName,
                      const std::string& vHelp);
    void addSignalValue(const std::string& vCategory, const std::string& vName, double vEpoch, double vValue);
    void addSignalStatus(const std::string& vCategory, const std::string& vName, double vEpoch, const std::string& vStatus);
    void addSignalStartZone(const std::string& vCategory, const std::string& vName, double vEpoch, const std::string& vStartMsg);
    void addSignalEndZone(const std::string& vCategory, const std::string& vName, double vEpoch, const std::string& vEndMsg);
};

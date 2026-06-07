#pragma once

// sol2 config (SOL_ALL_SAFETIES_ON, SOL_EXCEPTIONS_SAFE_PROPAGATION) is injected
// by the plugin's CMakeLists for every TU.
#include <sol/sol.hpp>

#include <apis/LtgPluginApi.h>
#include <modules/LuaRegex.h>
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
    int32_t m_RowIndex = 0;
    int32_t m_RowCount = 0;

public:
    void setRowIndex(int32_t vRowIndex);
    void setRowCount(int32_t vRowCount);

public:
    double luaModuleGetRowIndex();
    double luaModuleGetRowCount();
    void luaModuleLogInfo(const std::string& vKey);
    void luaModuleLogWarning(const std::string& vKey);
    void luaModuleLogError(const std::string& vKey);
    void luaModuleLogDebug(const std::string& vKey);
    double luaModuleStringToEpoch(const std::string& vDateTime, double vHourOffset);
    std::string luaModuleEpochToString(double vEpochTime, double vHourOffset);
    void luaModuleAddSignalTag(double vEpoch, double r, double g, double b, double a, const std::string& vName, const std::string& vHelp);
    void luaModuleAddSignalValue(const std::string& vCategory, const std::string& vName, double vEpoch, double vValue);
    void luaModuleAddSignalValueWithDesc(const std::string& vCategory, const std::string& vName, double vEpoch, double vValue, const std::string&);
    void luaModuleAddSignalStatus(const std::string& vCategory, const std::string& vName, double vEpoch, const std::string& vStatus);
    void luaModuleAddSignalStartZone(const std::string& vCategory, const std::string& vName, double vEpoch, const std::string& vStartMsg);
    void luaModuleAddSignalEndZone(const std::string& vCategory, const std::string& vName, double vEpoch, const std::string& vEndMsg);
    // factory for the shared regex brick — `ltg:regex(pattern)` returns a LuaRegex usertype, which
    // the user then keeps as a local/global and calls `:test` / `:match` / `:find` / `:gsub` /
    // `:gmatch` on. Typical usage compiles a handful of patterns at script init and reuses them
    // across every row of every log file. boost::regex_error on an invalid pattern is caught by
    // sol2's exception_handler and surfaces e.what() (real message, not "C++ exception").
    LuaRegex luaModuleRegex(const std::string& vPattern) const;
};

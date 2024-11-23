#pragma once

#include <apis/StrockerPluginApi.h>

#include <apis/CurlWrapper.h>
#include <Modules/YahooApi.hpp>
#include <Settings/Settings.h>

#include <json.hpp>

#include <ImGuiPack.h>

#include <string>
#include <vector>

class Module : public Sto::DataBrockerModule, public Sto::IGuiDrawer {
public:
    static Sto::DataBrockerModulePtr create(const SettingsWeak& vSettings);

private:
    YahooApi m_Api;
    CurlWrapper m_CurlWrapper;
    std::string m_RequestRawDatas;
    nlohmann::json m_RequestJsonDatas = {}; 
    Sto::SymbolPrices m_RequestPricesDatas; 
    SettingsWeak m_Settings;

    ImWidgets::QuickDateTime m_StartDate;
    ImWidgets::QuickDateTime m_EndDate;
    ImWidgets::QuickStringCombo m_Range;
    ImWidgets::QuickStringCombo m_Interval;
    ImWidgets::QuickStringCombo m_OptionsCombo;
    enum class Options {
        INTERVAL_FROM_CURRENT_DATE_WITH_RANGE = 0,
        INTERVAL_FROM_DATE_WITH_RANGE,
        INTERVAL_FROM_DATE_TO_DATE,
        Count
    } m_SelectedOptions = Options::INTERVAL_FROM_CURRENT_DATE_WITH_RANGE;
    ImWidgets::QuickStringCombo m_RangeCombo;
    YahooApi::Range m_SelectedRange = YahooApi::Range::_1d;
    ImWidgets::QuickStringCombo m_IntervalCombo;
    YahooApi::Interval m_SelectedInterval = YahooApi::Interval::_5m;

public:
    virtual ~Module() = default;
    bool init(Sto::PluginBridge* vBridgePtr = nullptr) override;
    void unit() override;
    bool DrawWidgets(const uint32_t& vCurrentFrame, ImGuiContext* vContextPtr, void* vUserDatas) override;
    bool DrawDialogsAndPopups(const uint32_t& vCurrentFrame, const ImRect& vMaxRect, ImGuiContext* vContextPtr, void* vUserDatas) override;
    bool StartPluginConfigRequest() override;
    bool Request(const std::string& vSymbol, const Sto::ProtocolType vProtocol) override;
    bool GrabOneDay(const std::string& vSymbol, const Sto::IntervalType vInterval, const Sto::RangeType vRange) override;
    Sto::SymbolPrices getLastRequestedPrices() override;

private:
    Sto::SymbolPrices m_parseJsonResponse(const std::string& vResponse);
    Sto::SymbolPrices m_parseJsonResponseHistorical(const nlohmann::json& vJsonParsed);
    std::time_t m_convertToEpochTime(const std::string& vIsoDateTime, const char* format = "%Y-%m-%d");
    std::string m_convertToISO8601(const std::time_t& vEpochTime);
};

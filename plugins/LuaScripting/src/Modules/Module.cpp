#include "Module.h"
#include <Panes/DataPane.h>

#include <ImGuiPack.h>

#include <EzLibs/EzFile.hpp>
#include <EzLibs/EzTime.hpp>

#include <chrono>
#include <ctime>

using json = nlohmann::json;

Sto::DataBrockerModulePtr Module::create(const SettingsWeak& vSettings) {
    assert(!vSettings.expired());
    auto res = std::make_shared<Module>();
    res->m_Settings = vSettings;
    if (!res->init()) {
        res.reset();
    }
    return res;
}

bool Module::init(Sto::PluginBridge* vBridgePtr) {
    m_OptionsCombo = ImWidgets::QuickStringCombo((int32_t)m_SelectedOptions, {"Range", "From date with range", "From date to date"});
    std::vector<std::string> ranges;
    ranges.reserve(m_Api.m_RangeStrings.size());
    for (const auto& a : m_Api.m_RangeStrings) {
        ranges.push_back(a);
    }
    m_RangeCombo = ImWidgets::QuickStringCombo((int32_t)m_SelectedRange, ranges);
    std::vector<std::string> intervals;
    intervals.reserve(m_Api.m_IntervalStrings.size());
    for (const auto& a : m_Api.m_IntervalStrings) {
        intervals.push_back(a);
    }
    m_IntervalCombo = ImWidgets::QuickStringCombo((int32_t)m_SelectedInterval, intervals);
    return m_CurlWrapper.init();
}

void Module::unit() {
    m_CurlWrapper.unit();
}

bool Module::DrawWidgets(const uint32_t& vCurrentFrame, ImGuiContext* vContextPtr, void* vUserDatas) {
    bool res = false;

    const float column_offset = 80.0f;

    if (m_OptionsCombo.displayWithColumn(0.0f, "Mode", column_offset)) {
        m_SelectedOptions = (Options)m_OptionsCombo.getIndex();
        res = true;
    }

    if (m_IntervalCombo.displayWithColumn(0.0f, "Interval", column_offset)) {
        m_SelectedInterval = (YahooApi::Interval)m_IntervalCombo.getIndex();
        res = true;
    }

    if (m_SelectedOptions == Options::INTERVAL_FROM_CURRENT_DATE_WITH_RANGE) {
        if (m_RangeCombo.displayWithColumn(0.0f, "Range", column_offset)) {
            m_SelectedRange = (YahooApi::Range)m_RangeCombo.getIndex();
            res = true;
        }
    } else if (m_SelectedOptions == Options::INTERVAL_FROM_DATE_WITH_RANGE) {
        if (m_StartDate.displayWithColumn(0.0f, "Start date", column_offset)) {
            res = true;
        }
        if (m_RangeCombo.displayWithColumn(0.0f, "Range", column_offset)) {
            m_SelectedRange = (YahooApi::Range)m_RangeCombo.getIndex();
            res = true;
        }   
    } else if (m_SelectedOptions == Options::INTERVAL_FROM_DATE_TO_DATE) {
        if (m_StartDate.displayWithColumn(0.0f, "Start date", column_offset)) {
            res = true;
        }
        if (m_EndDate.displayWithColumn(0.0f, "End date", column_offset)) {
            res = true;
        }
    }

    return res;
}

bool Module::DrawDialogsAndPopups(const uint32_t& vCurrentFrame, const ImRect& vMaxRect, ImGuiContext* vContextPtr, void* vUserDatas) {
    return false;
}

bool Module::StartPluginConfigRequest() {
    m_RequestRawDatas = ez::file::loadFileToString("yahoo_historic_datas_CURLE_OK.txt");
    DataPane::Instance()->setRequestDatasRaw(m_RequestRawDatas);
    m_RequestPricesDatas = m_parseJsonResponse(m_RequestRawDatas);
    if (!m_RequestPricesDatas.prices.times.empty()) {
        DataPane::Instance()->setRequestDatasJson(m_RequestJsonDatas);
        DataPane::Instance()->setRequestDatasPrices(m_RequestPricesDatas);
        return true;
    }
    return false;
}

bool Module::Request(const std::string& vSymbol, const Sto::ProtocolType vProtocol) {
    std::stringstream file_name;
    file_name << "yahoo_json_" << vSymbol << ".txt";
    if (vProtocol == Sto::ProtocolType::NET) {
        std::string url;
        if (m_SelectedOptions == Options::INTERVAL_FROM_CURRENT_DATE_WITH_RANGE) {
            url = m_Api.getUrlForHistory(vSymbol, m_SelectedInterval, m_SelectedRange);
        } else if (m_SelectedOptions == Options::INTERVAL_FROM_DATE_WITH_RANGE) {
            url = m_Api.getUrlForHistory(vSymbol, m_SelectedInterval, m_StartDate.getDateAsEpoch(), m_SelectedRange);
        } else if (m_SelectedOptions == Options::INTERVAL_FROM_DATE_TO_DATE) {
            url = m_Api.getUrlForHistory(vSymbol, m_SelectedInterval, m_StartDate.getDateAsEpoch(), m_EndDate.getDateAsEpoch());
        }
        std::string error_code;
        m_RequestRawDatas = m_CurlWrapper.DownloadDatas(url, error_code);
        ez::file::saveStringToFile(m_RequestRawDatas, file_name.str());
        DataPane::Instance()->setRequestDatasRaw(m_RequestRawDatas);
        m_RequestPricesDatas = m_parseJsonResponse(m_RequestRawDatas);
        if (!m_RequestPricesDatas.prices.times.empty()) {
            DataPane::Instance()->setRequestDatasJson(m_RequestJsonDatas);
            DataPane::Instance()->setRequestDatasPrices(m_RequestPricesDatas);
            return true;
        }
    } else if (vProtocol == Sto::ProtocolType::FILE) {
        m_RequestRawDatas = ez::file::loadFileToString(file_name.str());
        DataPane::Instance()->setRequestDatasRaw(m_RequestRawDatas);
        try {
            m_RequestPricesDatas = m_parseJsonResponse(m_RequestRawDatas);
        } catch (...) {
            return false;
        }
        if (!m_RequestPricesDatas.prices.times.empty()) {
            DataPane::Instance()->setRequestDatasJson(m_RequestJsonDatas);
            DataPane::Instance()->setRequestDatasPrices(m_RequestPricesDatas);
            return true;
        }
    }
    return false;
}

bool Module::GrabOneDay(const std::string& vSymbol, const Sto::IntervalType vInterval, const Sto::RangeType vRange) {
    EZ_TOOLS_DEBUG_BREAK;
    return false;
}

Sto::SymbolPrices Module::getLastRequestedPrices() {
    return m_RequestPricesDatas;
}

/////////////////////////////////////////////////
///// PRIVATE ///////////////////////////////////
/////////////////////////////////////////////////

Sto::SymbolPrices Module::m_parseJsonResponse(const std::string& vResponse) {
    m_RequestJsonDatas = json::parse(vResponse);
    return m_parseJsonResponseHistorical(m_RequestJsonDatas);
}

Sto::SymbolPrices Module::m_parseJsonResponseHistorical(const nlohmann::json& vJsonParsed) {
    Sto::SymbolPrices res;
    if (!vJsonParsed.is_null()) {
        if (vJsonParsed.contains("chart")) {
            const auto& root_chart = vJsonParsed["chart"];
            if (root_chart.contains("result")) {
                const auto& root_chart_result = root_chart["result"];
                if (root_chart_result.is_array() && root_chart_result.size() == 1U) {
                    const auto& item = root_chart_result[0];
                    if (item.contains("meta")) {
                        const auto& meta = item["meta"];
                        if (!meta["fullExchangeName"].is_null()) {
                            res.market =  meta["fullExchangeName"];
                        }
                        if (!meta["symbol"].is_null()) {
                            res.symbol = meta["symbol"];
                        }
                        if (!meta["dataGranularity"].is_null()) {
                            const std::string interval = meta["dataGranularity"];
                            YahooApi yapi;
                            size_t idx = 0U;
                            for (const auto& a : yapi.m_IntervalStrings) {
                                if (a == interval) {
                                    res.prices.period = yapi.m_IntervalInMins.at(idx);
                                }
                                ++idx;
                            }
                        }
                    }
                    if (item.contains("timestamp")) {
                        const auto& timestamps = item["timestamp"];
                        if (item.contains("indicators")) {
                            const auto& ind = item["indicators"];
                            if (ind.contains("quote")) {
                                const auto& quotes = ind["quote"][0];
                                if (!quotes.is_null()) {
                                    int32_t idx = 0;
                                    for (size_t i = 0; i < timestamps.size(); ++i) {
                                        if (!timestamps[i].is_null() &&       //
                                            !quotes["open"][i].is_null() &&   //
                                            !quotes["high"][i].is_null() &&   //
                                            !quotes["low"][i].is_null() &&    //
                                            !quotes["close"][i].is_null() &&  //
                                            !quotes["volume"][i].is_null()) {
                                            res.prices.times.push_back(timestamps[i].get<double>());
                                            res.prices.opens.push_back(quotes["open"][i].get<double>());
                                            res.prices.highs.push_back(quotes["high"][i].get<double>());
                                            res.prices.lows.push_back(quotes["low"][i].get<double>());
                                            res.prices.closes.push_back(quotes["close"][i].get<double>());
                                            res.prices.volumes.push_back(quotes["volume"][i].get<double>());
                                            res.prices.indexs.push_back(static_cast<double>(idx++));
                                        } else {
                                            LogVarDebugWarning("Bar[%i] is null ", (int32_t)i);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return res;
}

std::time_t Module::m_convertToEpochTime(const std::string& vIsoDateTime, const char* format) {
    struct std::tm time = {0, 0, 0, 0, 0, 0, 0, 0, 0};
    std::istringstream ss(vIsoDateTime);
    ss >> std::get_time(&time, format);
    if (ss.fail()) {
        std::cerr << "ERROR: Cannot parse date string (" << vIsoDateTime << "); required format %Y-%m-%d" << std::endl;
        exit(1);
    }
    time.tm_hour = 0;
    time.tm_min = 0;
    time.tm_sec = 0;
#ifdef _MSC_VER
    return _mkgmtime(&time);
#else
    return timegm(&time);
#endif
}

std::string Module::m_convertToISO8601(const std::time_t& vEpochTime) {
    std::string ret;
    auto tp = std::chrono::system_clock::from_time_t(vEpochTime);
    auto tt = std::chrono::system_clock::to_time_t(tp);
#ifdef _MSC_VER
    tm _timeinfo;
    tm* tm_timeinfo = &_timeinfo;
    if (localtime_s(tm_timeinfo, &tt) != 0) {
        return ret;
    }
#else
    auto* tm_timeinfo = std::localtime(&tt);
#endif
    std::ostringstream oss;
    oss << std::put_time(tm_timeinfo, "%Y-%m-%dT%H:%M:%S");
    ret = oss.str();
    return ret;
}


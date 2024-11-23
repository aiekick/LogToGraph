/*
Copyright 2022-2024 Stephane Cuillerdier (aka aiekick)

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#pragma once
#pragma warning(disable : 4251)

#include <memory>
#include <cstdint>
#include <string>
#include <vector>
#include <array>
#include <map>

#include "ILayoutPane.h"

namespace Ltg {

class ProjectInterface {
public:
    virtual bool IsProjectLoaded() const = 0;
    virtual bool IsProjectNeverSaved() const = 0;
    virtual bool IsThereAnyProjectChanges() const = 0;
    virtual void SetProjectChange(bool vChange = true) = 0;
    virtual bool WasJustSaved() = 0;
};
typedef std::shared_ptr<ProjectInterface> ProjectInterfacePtr;
typedef std::weak_ptr<ProjectInterface> ProjectInterfaceWeak;

struct PluginPane : public virtual ILayoutPane {
    bool Init() override = 0;  // return false if the init was failed
    void Unit() override = 0;

    // the return, is a user side use case here
    bool DrawPanes(const uint32_t& vCurrentFrame, bool* vOpened, ImGuiContext* vContextPt, void* vUserDatas) override = 0;
    bool DrawWidgets(const uint32_t& /*vCurrentFrame*/, ImGuiContext* /*vContextPtr*/, void* /*vUserDatas*/) override {
        return false;
    }
    bool DrawOverlays(const uint32_t& /*vCurrentFrame*/, const ImRect& /*vRect*/, ImGuiContext* /*vContextPtr*/, void* /*vUserDatas*/) override {
        return false;
    }
    bool DrawDialogsAndPopups(const uint32_t& /*vCurrentFrame*/, const ImRect& /*vMaxRect*/, ImGuiContext* /*vContextPtr*/, void* /*vUserDatas*/) override {
        return false;
    }

    // if for any reason the pane must be hidden temporary, the user can control this here
    virtual bool CanBeDisplayed() override = 0;

    virtual void SetProjectInstance(ProjectInterfaceWeak vProjectInstance) = 0;
};

struct PluginPaneConfig {
    ILayoutPaneWeak pane;
    std::string name;
    std::string category;
    std::string disposal = "CENTRAL";
    float disposalRatio = 0.0f;
    bool openedDefault = false;
    bool focusedDefault = false;
};

typedef std::string SettingsCategoryPath;
enum class ISettingsType {
    NONE = 0,
    APP,     // common for all users
    PROJECT  // user specific
};

struct IXmlSettings {
    // will be called by the saver. userdatas will have two possible values. APP or PROJECT. PROJECT mean user side, APP mean common for all users
    virtual std::string GetXmlSettings(const std::string& vOffset, const ISettingsType& vType) const = 0;
    // will be called by the loader0 userdatas will have two possible values. APP or PROJECT. PROJECT mean user side, APP mean common for all users
    virtual void SetXmlSettings(const std::string& vName, const std::string& vParentName, const std::string& vValue, const ISettingsType& vType) = 0;
};

struct IGuiDrawer {
    virtual bool DrawWidgets(const uint32_t& vCurrentFrame, ImGuiContext* vContextPtr, void* vUserDatas) = 0;
    virtual bool DrawDialogsAndPopups(const uint32_t& vCurrentFrame, const ImRect& vMaxRect, ImGuiContext* vContextPtr, void* vUserDatas) = 0;
};

struct ChartColor {
    float r = 0.0f;
    float g = 0.0;
    float b = 0.0f;
    float a = 0.0f;
};

struct ChartPoint {
    double x = 0.0;
    double y = 0.0;
};

struct ChartBox {
    ChartPoint min;
    ChartPoint max;
    ChartColor col;
};

struct ChartSegment {
    ChartPoint a;
    ChartPoint b;
    ChartColor col;
    float thickness = 0.0;
};

typedef std::vector<double> Serie;
struct Prices {
    Serie opens;
    Serie highs;
    Serie lows;
    Serie closes;
    Serie volumes;
    Serie times;
    Serie indexs;
    uint32_t period = 0;  // interval in minutes
    bool noDateTime = false;
};

struct SymbolPrices {
    Prices prices;
    std::string market;
    std::string symbol;
};

struct ChartColumnStyle {
    int32_t upper_normal = 0;    // ImU32
    int32_t down_normal = 0;     // ImU32
    int32_t upper_hovered = 0;   // ImU32
    int32_t down_hovered = 0;    // ImU32
    int32_t column_hovered = 0;  // ImU32
    int32_t grid_normal = 0;     // ImU32
    int32_t text_normal = 0;     // ImU32
};

struct ChartColumn {
    int32_t last_indx = -1;
    int32_t curr_indx = -1;
    double half_width = 0.0;
    bool last_hovered = false;
    bool curr_hovered = false;
};

struct IChartPrices {
    virtual const Prices* getPrices() const = 0;
};

struct IChartStyle {
    virtual const ChartColumnStyle* getStyle() const = 0;
};

struct PluginParam {
    std::string name;
    enum class Type { NUM, STRING } type = Type::NUM;
    double valueD = 0.0;
    std::string valueS;
    explicit PluginParam(const std::string& vName, const double& vValue) : name(vName), type(Type::NUM), valueD(vValue) {
    }
    explicit PluginParam(const std::string& vName, const std::string& vValue) : name(vName), type(Type::NUM), valueS(vValue) {
    }
};
typedef std::vector<PluginParam> PluginParams;

struct IndicatorComputing {
    virtual bool compute(const Prices& vPrices) = 0;
    virtual const Serie* computeAndGetSerie(const Prices& vPrices, const uint32_t& vSerieIdx, PluginParams vParams) = 0;
};

typedef std::shared_ptr<IndicatorComputing> IndicatorComputingPtr;
typedef std::weak_ptr<IndicatorComputing> IndicatorComputingWeak;

struct PluginBridge {
    virtual IndicatorComputingPtr getIndicatorPtr(const std::string& vIndicatorName) = 0;
};

struct PluginModule {
    virtual ~PluginModule() = default;
    virtual bool init(PluginBridge* vBridgePtr) = 0;
    virtual void unit() = 0;
};

typedef std::shared_ptr<PluginModule> PluginModulePtr;
typedef std::weak_ptr<PluginModule> PluginModuleWeak;

struct IChartColumn : public IChartPrices, public IChartStyle {};

typedef std::array<double, 4> IChartV4;
struct ChartingModule : public PluginModule {
    virtual ~ChartingModule() = default;
    // used for recompute prices datas
    virtual Prices computePrices(const Prices& vPrices) = 0;
    // draw a column
    virtual bool DrawColumn(const ChartColumn& vChartColumn, const IChartColumn& vColumnDrawer) = 0;
};

typedef std::shared_ptr<ChartingModule> ChartingModulePtr;
typedef std::weak_ptr<ChartingModule> ChartingModuleWeak;

enum class IndicatorSettingsType { PANE = 0, MENU };

struct IndicatorModule : public virtual PluginModule, public virtual IndicatorComputing {
public:
    virtual bool newFrame() = 0;
    virtual bool needUpdate() = 0;
    virtual const char* getLabel() = 0;
    virtual bool draw(const Ltg::Prices& vPrices) = 0;
    virtual bool drawSettings(const IndicatorSettingsType& vType) = 0;
};

typedef std::shared_ptr<IndicatorModule> IndicatorModulePtr;
typedef std::weak_ptr<IndicatorModule> IndicatorModuleWeak;

enum class ProtocolType { NET = 0, FILE, Count };

enum class IntervalType { _1m = 0, _2m, _3m, _5m, _10m, _15m, _30m, _45m, _1h, _2h, _3h, _4h, _1d, _1w, _1mo, Count };
static std::array<std::string, (size_t)IntervalType::Count> s_IntervalStrings = {
    "1m",
    "2m",
    "3m",
    "5m",
    "10m",
    "15m",
    "30m",
    "45m",
    "1h",
    "2h",
    "3h",
    "4h",
    "1d",
    "1w",
    "1mo",
};

enum class RangeType { _1d = 0, _5d, _1mo, _3mo, _6mo, _1y, _2y, _5y, _10y, _max, Count };
static std::array<std::string, (size_t)RangeType::Count> s_RangeStrings = {
    "1d",
    "5d",
    "1mo",
    "3mo",
    "6mo",
    "1y",
    "2y",
    "5y",
    "10y",
    "max",
};

struct DataBrockerModule : public PluginModule {
    // will start the request iof the config was mahe via the plugin imgui pane
    virtual bool StartPluginConfigRequest() = 0;
    // will start a request for price of a symbol in a date range
    virtual bool Request(const std::string& vSymbol, const ProtocolType vProtocol) = 0;
    // dataz grabber for server. the interval must be converted
    virtual bool GrabOneDay(const std::string& vSymbol, const IntervalType vInterval, const RangeType vRange) = 0;
    virtual SymbolPrices getLastRequestedPrices() = 0;
};

typedef std::shared_ptr<DataBrockerModule> DataBrockerModulePtr;
typedef std::weak_ptr<DataBrockerModule> DataBrockerModuleWeak;

enum class PluginModuleType { NONE = 0, DATA_BROKER, CHARTING, INDICATOR, Count };

struct PluginModuleInfos {
    std::string path;
    std::string label;
    std::map<std::string, std::string> dico;
    PluginModuleType type;
    std::array<float, 4> color{};
    PluginModuleInfos(const std::string& vPath, const std::string& vLabel, const PluginModuleType& vType, const std::array<float, 4>& vColor = {})
        : path(vPath), label(vLabel), type(vType), color(vColor) {
    }
};

struct ISettings : public IXmlSettings {
public:
    // get the categroy path of the settings for the mebnu display. ex: "plugins/apis"
    virtual SettingsCategoryPath GetCategory() const = 0;
    // will be called by the loader for inform the pluign than he must load somethings if any
    virtual bool LoadSettings() = 0;
    // will be called by the saver for inform the pluign than he must save somethings if any, by ex: temporary vars
    virtual bool SaveSettings() = 0;
    // will draw custom settings via imgui
    virtual bool DrawSettings() = 0;
};

typedef std::shared_ptr<ISettings> ISettingsPtr;
typedef std::weak_ptr<ISettings> ISettingsWeak;

struct PluginSettingsConfig {
    ISettingsWeak settings;
    PluginSettingsConfig(ISettingsWeak vSertings) : settings(vSertings) {
    }
};

struct PluginInterface {
    PluginInterface() = default;
    virtual ~PluginInterface() = default;
    virtual bool Init() = 0;
    virtual void Unit() = 0;
    virtual uint32_t GetMinimalStrockerVersionSupported() const = 0;
    virtual uint32_t GetVersionMajor() const = 0;
    virtual uint32_t GetVersionMinor() const = 0;
    virtual uint32_t GetVersionBuild() const = 0;
    virtual std::string GetName() const = 0;
    virtual std::string GetAuthor() const = 0;
    virtual std::string GetVersion() const = 0;
    virtual std::string GetContact() const = 0;
    virtual std::string GetDescription() const = 0;
    virtual std::vector<PluginModuleInfos> GetModulesInfos() const = 0;
    virtual PluginModulePtr CreateModule(const std::string& vPluginModuleName, Ltg::PluginBridge* vBridgePtr) = 0;
    virtual std::vector<PluginPaneConfig> GetPanes() const = 0;
    virtual std::vector<PluginSettingsConfig> GetSettings() const = 0;
};

}  // namespace Sto
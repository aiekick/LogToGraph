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
#include <ezlibs/ezXml.hpp>

namespace ez {
class Log;
}

namespace Ltg {

class IProject {
public:
    virtual bool IsProjectLoaded() const = 0;
    virtual bool IsProjectNeverSaved() const = 0;
    virtual bool IsThereAnyProjectChanges() const = 0;
    virtual void SetProjectChange(bool vChange = true) = 0;
    virtual bool WasJustSaved() = 0;
};
typedef std::shared_ptr<IProject> IProjectPtr;
typedef std::weak_ptr<IProject> IProjectWeak;

struct PluginPane : public virtual ILayoutPane {
    bool Init() override = 0;  // return false if the init was failed
    void Unit() override = 0;

    // the return, is a user side use case here
    bool DrawPanes(const uint32_t& vCurrentFrame, bool* vOpened, ImGuiContext* vContextPt, void* vUserDatas) override = 0;
    bool DrawWidgets(const uint32_t& /*vCurrentFrame*/, ImGuiContext* /*vContextPtr*/, void* /*vUserDatas*/) override { return false; }
    bool DrawOverlays(const uint32_t& /*vCurrentFrame*/, const ImRect& /*vRect*/, ImGuiContext* /*vContextPtr*/, void* /*vUserDatas*/) override { return false; }
    bool DrawDialogsAndPopups(const uint32_t& /*vCurrentFrame*/, const ImRect& /*vMaxRect*/, ImGuiContext* /*vContextPtr*/, void* /*vUserDatas*/) override {
        return false;
    }

    // if for any reason the pane must be hidden temporary, the user can control this here
    virtual bool CanBeDisplayed() override = 0;

    virtual void SetProjectInstance(IProjectWeak vProjectInstance) = 0;
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
    // will be called by the saver
    virtual ez::xml::Nodes getXmlSettings(const ISettingsType& vType) const = 0;
    // will be called by the loader
    virtual void setXmlSettings(const ez::xml::Node& vName, const ez::xml::Node& vParent, const std::string& vValue, const ISettingsType& vType) = 0;
};

struct PluginParam {
    std::string name;
    enum class Type { NUM, STRING } type = Type::NUM;
    double valueD = 0.0;
    std::string valueS;
    explicit PluginParam(const std::string& vName, const double& vValue) : name(vName), type(Type::NUM), valueD(vValue) {}
    explicit PluginParam(const std::string& vName, const std::string& vValue) : name(vName), type(Type::NUM), valueS(vValue) {}
};
typedef std::vector<PluginParam> PluginParams;

struct PluginBridge {};

struct PluginModule {
    virtual ~PluginModule() = default;
    virtual bool init(PluginBridge* vBridgePtr) = 0;
    virtual void unit() = 0;
};

typedef std::shared_ptr<PluginModule> PluginModulePtr;
typedef std::weak_ptr<PluginModule> PluginModuleWeak;

enum class PluginModuleType { NONE = 0, SCRIPTING, Count };

struct PluginModuleInfos {
    std::string path;
    std::string label;
    std::map<std::string, std::string> dico;
    PluginModuleType type;
    std::array<float, 4> color{};
    PluginModuleInfos(const std::string& vPath, const std::string& vLabel, const PluginModuleType& vType, const std::array<float, 4>& vColor = {})
        : path(vPath), label(vLabel), type(vType), color(vColor) {}
};

struct ISettings : public IXmlSettings {
    virtual ~ISettings() = default;
    // get the category path of the settings for the mebnu display. ex: "plugins/apis"
    virtual SettingsCategoryPath getCategory() const = 0;
    // will be called by the loader for inform the pluign than he must load somethings if any
    virtual bool loadSettings() = 0;
    // will be called by the saver for inform the pluign than he must save somethings if any, by ex: temporary vars
    virtual bool saveSettings() = 0;
    // will draw custom settings via imgui
    virtual bool drawSettings() = 0;
};

typedef std::shared_ptr<ISettings> ISettingsPtr;
typedef std::weak_ptr<ISettings> ISettingsWeak;

typedef std::string ScriptFilePathName;

struct ScriptingError {
    ScriptFilePathName file;
    size_t line = 0;
    size_t column = 0;
};
typedef std::vector<ScriptingError> ErrorContainer;

// lua_register(lua_state_ptr, "print", lua_int_print_args);
struct IDatasModel {
    // is the entry point of the script. this function is needed
    //virtual void init() = 0;
    // will set the description of your script in app
    //virtual void setScriptDescription(const std::string& vKey) = 0;
    // set the lua string varaible name who will be filled with the content of the row file
    //virtual void setRowBufferName(const std::string& vKey) = 0;
    // set the function name who will be called at each row of the file
    //virtual void setFunctionForEachRow(const std::string& vKey) = 0;
    // set the function name who will be called at the end of the file
    //virtual void setFunctionForEndFile(const std::string& vKey) = 0;

    // will log the message in the in app console
    //virtual void logInfo(const std::string& vKey) = 0;
    // will log the message in the in app console
    //virtual void logWarning(const std::string& vKey) = 0;
    // will log the message in the in app console
    //virtual void logError(const std::string& vKey) = 0;
    // will log the message in the in app console
    //virtual void logDebug(const std::string& vKey) = 0;

    // return the row number of the file
    //virtual int32_t getRowIndex() = 0;
    // return the number of rows of the file
    //virtual int32_t getRowCount() = 0;
    // get epoch time from datetime in format "YYYY-MM-DD HH:MM:SS,MS" or
    // "YYYY-MM-DD HH:MM:SS.MS" with hour offset in second param
    //virtual std::string getEpochTime(const std::string& vDateTime, int32_t vHourOffset) = 0;

    // Add Signal ticks
    // add a signal tag with date, color a name. the help will be displayed when mouse over the tag
    // rgba are normalized [0.0:1.0]
    virtual void addSignalTag(double vEpoch, double r, double g, double b, double a, const std::string& vName, const std::string& vHelp) = 0;
    // will add a signal string status
    virtual void addSignalStatus(const std::string& vCategory, const std::string& vName, double vEpoch, const std::string& vStatus) = 0;
    // will add a signal numerical value
    virtual void addSignalValue(const std::string& vCategory, const std::string& vName, double vEpoch, double vValue) = 0;
    // will add a signal start zone
    virtual void addSignalStartZone(const std::string& vCategory, const std::string& vName, double vEpoch, const std::string& vStartMsg) = 0;
    // will add a signal end zone
    virtual void addSignalEndZone(const std::string& vCategory, const std::string& vName, double vEpoch, const std::string& vEndMsg) = 0;
};

typedef std::shared_ptr<IDatasModel> IDatasModelPtr;
typedef std::weak_ptr<IDatasModel> IDatasModelWeak;

struct ScriptingDatas {
    std::string buffer;
};
typedef std::string ScriptingModuleName;
struct ScriptingModule : public PluginModule {
    virtual ~ScriptingModule() = default;
    // will load the related scripting engine
    virtual bool load(IDatasModelWeak vDatasModel) = 0;
    // will unload the related scripting engine
    virtual void unload() = 0;
    // will compile the script and return errors
    virtual bool compileScript(const ScriptFilePathName& vFilePathName, ErrorContainer& vOutErrors) = 0;
    // will call the start function from script and return errors
    virtual bool callScriptStart(ErrorContainer& vOutErrors) = 0;
    // will call the exec function from script with a buffer and return errors
    virtual bool callScriptExec(const ScriptingDatas& vOutDatas, ErrorContainer& vErrors) = 0;
    // will call the end function from script and return errors
    virtual bool callScriptEnd(ErrorContainer& vOutErrors) = 0;
};

typedef std::shared_ptr<ScriptingModule> ScriptingModulePtr;
typedef std::weak_ptr<ScriptingModule> ScriptingModuleWeak;

struct PluginSettingsConfig {
    ISettingsWeak settings;
    PluginSettingsConfig(ISettingsWeak vSertings) : settings(vSertings) {}
};

struct PluginInterface {
    virtual ~PluginInterface() = default;
    virtual bool init(ez::Log* vLoggerInstancePtr) = 0;
    virtual void unit() = 0;
    virtual uint32_t getMinimalAppVersionSupported() const = 0;
    virtual uint32_t getVersionMajor() const = 0;
    virtual uint32_t getVersionMinor() const = 0;
    virtual uint32_t getVersionBuild() const = 0;
    virtual std::string getName() const = 0;
    virtual std::string getAuthor() const = 0;
    virtual std::string getVersion() const = 0;
    virtual std::string getContact() const = 0;
    virtual std::string getDescription() const = 0;
    virtual std::vector<PluginModuleInfos> getModulesInfos() const = 0;
    virtual PluginModulePtr createModule(const std::string& vPluginModuleName, Ltg::PluginBridge* vBridgePtr) = 0;
    virtual std::vector<PluginPaneConfig> getPanes() const = 0;
    virtual std::vector<PluginSettingsConfig> getSettings() const = 0;
};

}  // namespace Ltg
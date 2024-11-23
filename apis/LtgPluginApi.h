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
    // will be called by the saver
    virtual ez::xml::Nodes GetXmlSettings(const ISettingsType& vType) const = 0;
    // will be called by the loader
    virtual void SetXmlSettings(const ez::xml::Node& vName, const ez::xml::Node& vParentName, const std::string& vValue, const ISettingsType& vType) = 0;
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


struct PluginBridge {
};

struct PluginModule {
    virtual ~PluginModule() = default;
    virtual bool init(PluginBridge* vBridgePtr) = 0;
    virtual void unit() = 0;
};

typedef std::shared_ptr<PluginModule> PluginModulePtr;
typedef std::weak_ptr<PluginModule> PluginModuleWeak;

enum class PluginModuleType { NONE = 0, Count };

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
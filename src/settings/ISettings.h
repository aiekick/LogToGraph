/*
Copyright 2022-2026 Stephane Cuillerdier (aka aiekick)

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

#include <string>
#include <memory>
#include <ezlibs/ezXml.hpp>

// Host-side settings interface, consumed by the app's SettingsDialog. It used to live in the
// plugin API (LtgPluginApi.h), but plugins no longer expose settings, so it moved here: the app's
// own components (which DO have settings to show) implement ISettings. Namespace kept as Ltg to
// avoid churn in the dialog; it can be renamed to a host namespace later.
namespace Ltg {

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

struct ISettings : public IXmlSettings {
    virtual ~ISettings() = default;
    // get the category path of the settings for the menu display. ex: "app/general"
    virtual SettingsCategoryPath getCategory() const = 0;
    // will be called by the loader to inform that something must be loaded if any
    virtual bool loadSettings() = 0;
    // will be called by the saver to inform that something must be saved if any
    virtual bool saveSettings() = 0;
    // will draw custom settings via imgui
    virtual bool drawSettings() = 0;
    // called by SettingsDialog::clearProjectSettings on New/Open/Close so per-project values get reset.
    // default no-op so APP-only implementers (e.g. AppSettings) need not override.
    virtual void clearProjectSettings() {}
};

typedef std::shared_ptr<ISettings> ISettingsPtr;
typedef std::weak_ptr<ISettings> ISettingsWeak;

}  // namespace Ltg

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

#include <ezlibs/ezClass.hpp>
#include <ezlibs/ezSingleton.hpp>
#include <settings/ISettings.h>

// App-level settings exposed via SettingsDialog. Persisted to the app's config.xml
// (APP scope), not the .ltg project. First member: the glfwWaitEventsTimeout used by
// MainBackend's main loop when idle — toggle + timeout in seconds.
class AppSettings : public Ltg::ISettings {
    DISABLE_CONSTRUCTORS(AppSettings)
    DISABLE_DESTRUCTORS(AppSettings)
    IMPLEMENT_SHARED_SINGLETON(AppSettings)

private:
    bool m_WaitEventsEnabled = true;
    double m_WaitEventsTimeoutSec = 1.0;
    double m_HoverDelaySec = 0.5;  // VS-style hover delay before the code editor shows a watch tooltip

public:
    // runtime accessors used by MainBackend::m_MainLoop
    bool isWaitEventsEnabled() const;
    double getWaitEventsTimeoutSec() const;
    // runtime accessor used by CodePane to gate the hover-eval tooltip
    double getHoverDelaySec() const;

    // Ltg::ISettings
    Ltg::SettingsCategoryPath getCategory() const final;
    bool loadSettings() final;
    bool saveSettings() final;
    bool drawSettings() final;
    ez::xml::Nodes getXmlSettings(const Ltg::ISettingsType& vType) const final;
    void setXmlSettings(const ez::xml::Node& vName, const ez::xml::Node& vParent, const std::string& vValue, const Ltg::ISettingsType& vType) final;
};

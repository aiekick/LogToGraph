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

// Per-feature toggles for the script-debugger / IDE features that ride on top of the editor:
// recompile-on-edit (the project-script autocomplete refresh, which can be expensive on big scripts —
// the original motivation for this dialog), autocomplete popup, signature help tooltip, hover-eval
// tooltip, error markers. All default to ON so the out-of-box experience matches the previous
// behaviour. Persisted in config.xml (APP scope), exposed via SettingsDialog under category
// "app/debug" AND via a compact Debug submenu in the CodePane's menu bar — both bind to the same
// fields so the two surfaces always agree.
class DebugSettings : public Ltg::ISettings {
    DISABLE_CONSTRUCTORS(DebugSettings)
    DISABLE_DESTRUCTORS(DebugSettings)
    IMPLEMENT_SHARED_SINGLETON(DebugSettings)

private:
    bool m_ProjectScriptRecompileEnabled = true;  // ScriptingEngine::SetProjectScriptCode push on every edit
    bool m_AutoCompletionEnabled = true;          // popup opened on `.` / `:` in the code editor
    bool m_SignatureHelpEnabled = true;           // tooltip opened on `(` in the code editor
    bool m_HoverEvalEnabled = true;               // value-on-hover tooltip when paused
    bool m_ErrorMarkersEnabled = true;            // red underlines + tooltips for last-run script errors
    bool m_AutoBreakpointOnError = false;         // opt-in: pause at the error line + auto-set a breakpoint there

public:
    bool isProjectScriptRecompileEnabled() const;
    bool isAutoCompletionEnabled() const;
    bool isSignatureHelpEnabled() const;
    bool isHoverEvalEnabled() const;
    bool isErrorMarkersEnabled() const;
    bool isAutoBreakpointOnErrorEnabled() const;

    // compact MenuItem-style rendering for the CodePane Debug submenu. Same underlying state as
    // drawSettings — toggling here is reflected in the settings dialog and vice versa.
    bool drawMenuItems();

    // Ltg::ISettings
    Ltg::SettingsCategoryPath getCategory() const final;
    bool loadSettings() final;
    bool saveSettings() final;
    bool drawSettings() final;
    ez::xml::Nodes getXmlSettings(const Ltg::ISettingsType& vType) const final;
    void setXmlSettings(const ez::xml::Node& vName, const ez::xml::Node& vParent, const std::string& vValue, const Ltg::ISettingsType& vType) final;
};

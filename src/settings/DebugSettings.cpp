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

#include "DebugSettings.h"

#include <imguipack.h>
#include <ezlibs/ezVariant.hpp>

#include <string>

bool DebugSettings::isProjectScriptRecompileEnabled() const {
    return m_ProjectScriptRecompileEnabled;
}

bool DebugSettings::isAutoCompletionEnabled() const {
    return m_AutoCompletionEnabled;
}

bool DebugSettings::isSignatureHelpEnabled() const {
    return m_SignatureHelpEnabled;
}

bool DebugSettings::isHoverEvalEnabled() const {
    return m_HoverEvalEnabled;
}

bool DebugSettings::isErrorMarkersEnabled() const {
    return m_ErrorMarkersEnabled;
}

bool DebugSettings::isAutoBreakpointOnErrorEnabled() const {
    return m_AutoBreakpointOnError;
}

Ltg::SettingsCategoryPath DebugSettings::getCategory() const {
    return "app/debug";
}

bool DebugSettings::loadSettings() {
    return true;
}

bool DebugSettings::saveSettings() {
    return true;
}

bool DebugSettings::drawSettings() {
    bool change = false;
    ImGui::TextUnformatted("Project script");
    if (ImGui::ToggleContrastedButton(
            "Recompile on edit: on",
            "Recompile on edit: off",
            &m_ProjectScriptRecompileEnabled,
            "Push the in-memory project script into the plugin's completion state on every edit.\n"
            "Disable for very large scripts where the per-edit sandbox re-exec is too costly —\n"
            "autocomplete then sees only the stdlib + ltg bindings, not user globals.")) {
        change = true;
    }

    ImGui::Separator();
    ImGui::TextUnformatted("Code editor IDE helpers");
    if (ImGui::ToggleContrastedButton(
            "Autocomplete popup: on", "Autocomplete popup: off", &m_AutoCompletionEnabled, "Open the autocomplete popup on `.` or `:` after a known identifier")) {
        change = true;
    }
    if (ImGui::ToggleContrastedButton(
            "Signature help: on", "Signature help: off", &m_SignatureHelpEnabled, "Open the signature-help tooltip on `(` after a known callable")) {
        change = true;
    }
    if (ImGui::ToggleContrastedButton(
            "Error markers: on", "Error markers: off", &m_ErrorMarkersEnabled, "Show red underlines + tooltips for last-run script errors")) {
        change = true;
    }

    ImGui::Separator();
    ImGui::TextUnformatted("Debugger");
    if (ImGui::ToggleContrastedButton(
            "Hover eval tooltip: on", "Hover eval tooltip: off", &m_HoverEvalEnabled, "Show a value tooltip when hovering an identifier while paused")) {
        change = true;
    }
    if (ImGui::ToggleContrastedButton(
            "Auto-bp + pause on error: on",
            "Auto-bp + pause on error: off",
            &m_AutoBreakpointOnError,
            "On a runtime script error, set a breakpoint at the error line and pause the worker\n"
            "synchronously with the throwing frame visible (locals, upvalues, call stack).\n"
            "Implies arming the debugger for this run.")) {
        change = true;
    }
    return change;
}

bool DebugSettings::drawMenuItems() {
    bool change = false;
    // ImGui::MenuItem("label", shortcut, bool*, enabled) — checkmark mirrors the bool, click toggles.
    if (ImGui::MenuItem("Recompile project script on edit", nullptr, &m_ProjectScriptRecompileEnabled)) {
        change = true;
    }
    ImGui::Separator();
    if (ImGui::MenuItem("Autocomplete popup", nullptr, &m_AutoCompletionEnabled)) {
        change = true;
    }
    if (ImGui::MenuItem("Signature help", nullptr, &m_SignatureHelpEnabled)) {
        change = true;
    }
    if (ImGui::MenuItem("Error markers", nullptr, &m_ErrorMarkersEnabled)) {
        change = true;
    }
    ImGui::Separator();
    if (ImGui::MenuItem("Hover eval tooltip", nullptr, &m_HoverEvalEnabled)) {
        change = true;
    }
    if (ImGui::MenuItem("Auto-bp + pause on error", nullptr, &m_AutoBreakpointOnError)) {
        change = true;
    }
    return change;
}

ez::xml::Nodes DebugSettings::getXmlSettings(const Ltg::ISettingsType& vType) const {
    if (vType != Ltg::ISettingsType::APP) {
        return {};
    }
    ez::xml::Node parent("debug_settings");
    parent.addChild("project_script_recompile_enabled").setContent(m_ProjectScriptRecompileEnabled ? "true" : "false");
    parent.addChild("autocompletion_enabled").setContent(m_AutoCompletionEnabled ? "true" : "false");
    parent.addChild("signature_help_enabled").setContent(m_SignatureHelpEnabled ? "true" : "false");
    parent.addChild("hover_eval_enabled").setContent(m_HoverEvalEnabled ? "true" : "false");
    parent.addChild("error_markers_enabled").setContent(m_ErrorMarkersEnabled ? "true" : "false");
    parent.addChild("auto_breakpoint_on_error").setContent(m_AutoBreakpointOnError ? "true" : "false");
    return {parent};
}

void DebugSettings::setXmlSettings(const ez::xml::Node& vName, const ez::xml::Node& vParent, const std::string& vValue, const Ltg::ISettingsType& vType) {
    if (vType != Ltg::ISettingsType::APP) {
        return;
    }
    if (vParent.getName() != "debug_settings") {
        return;
    }
    const auto& nodeName = vName.getName();
    if (nodeName == "project_script_recompile_enabled") {
        m_ProjectScriptRecompileEnabled = ez::ivariant(vValue).GetB();
    } else if (nodeName == "autocompletion_enabled") {
        m_AutoCompletionEnabled = ez::ivariant(vValue).GetB();
    } else if (nodeName == "signature_help_enabled") {
        m_SignatureHelpEnabled = ez::ivariant(vValue).GetB();
    } else if (nodeName == "hover_eval_enabled") {
        m_HoverEvalEnabled = ez::ivariant(vValue).GetB();
    } else if (nodeName == "error_markers_enabled") {
        m_ErrorMarkersEnabled = ez::ivariant(vValue).GetB();
    } else if (nodeName == "auto_breakpoint_on_error") {
        m_AutoBreakpointOnError = ez::ivariant(vValue).GetB();
    }
}

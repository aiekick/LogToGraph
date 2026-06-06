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

#include "AppSettings.h"

#include <imguipack.h>
#include <ezlibs/ezVariant.hpp>

#include <string>

bool AppSettings::isWaitEventsEnabled() const {
    return m_WaitEventsEnabled;
}

double AppSettings::getWaitEventsTimeoutSec() const {
    return m_WaitEventsTimeoutSec;
}

double AppSettings::getHoverDelaySec() const {
    return m_HoverDelaySec;
}

Ltg::SettingsCategoryPath AppSettings::getCategory() const {
    return "app/general";
}

bool AppSettings::loadSettings() {
    return true;
}

bool AppSettings::saveSettings() {
    return true;
}

bool AppSettings::drawSettings() {
    bool change = false;
    ImGui::TextUnformatted("Main loop idle wait");
    if (ImGui::ToggleContrastedButton("Wait events: on", "Wait events: off", &m_WaitEventsEnabled, "Block the main loop when idle to reduce CPU usage")) {
        change = true;
    }
    ImGui::BeginDisabled(!m_WaitEventsEnabled);
    if (ImGui::SliderDoubleDefault(200.0f, "Timeout (s)", &m_WaitEventsTimeoutSec, 0.01, 5.0, 1.0, 0.0, "%.3f")) {
        change = true;
    }
    ImGui::EndDisabled();

    ImGui::Separator();
    ImGui::TextUnformatted("Code editor watch-on-hover");
    if (ImGui::SliderDoubleDefault(200.0f, "Hover delay (s)", &m_HoverDelaySec, 0.0, 2.0, 0.5, 0.0, "%.2f")) {
        change = true;
    }
    return change;
}

ez::xml::Nodes AppSettings::getXmlSettings(const Ltg::ISettingsType& vType) const {
    if (vType != Ltg::ISettingsType::APP) {
        return {};
    }
    ez::xml::Node parent("app_settings");
    parent.addChild("wait_events_enabled").setContent(m_WaitEventsEnabled ? "true" : "false");
    parent.addChild("wait_events_timeout_sec").setContent(std::to_string(m_WaitEventsTimeoutSec));
    parent.addChild("hover_delay_sec").setContent(std::to_string(m_HoverDelaySec));
    return {parent};
}

void AppSettings::setXmlSettings(const ez::xml::Node& vName, const ez::xml::Node& vParent, const std::string& vValue, const Ltg::ISettingsType& vType) {
    if (vType != Ltg::ISettingsType::APP) {
        return;
    }
    if (vParent.getName() != "app_settings") {
        return;
    }
    const auto& nodeName = vName.getName();
    if (nodeName == "wait_events_enabled") {
        m_WaitEventsEnabled = ez::ivariant(vValue).GetB();
    } else if (nodeName == "wait_events_timeout_sec") {
        m_WaitEventsTimeoutSec = ez::ivariant(vValue).GetD();
    } else if (nodeName == "hover_delay_sec") {
        m_HoverDelaySec = ez::ivariant(vValue).GetD();
    }
}

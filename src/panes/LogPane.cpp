/*
Copyright 2022-2023 Stephane Cuillerdier (aka aiekick)

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

// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include "LogPane.h"
#include <panes/ToolPane.h>
#include <panes/SignalsHoveredMap.h>
#include <panes/GraphGroupPane.h>
#include <project/ProjectFile.h>
#include <cinttypes>  // printf zu

#include <models/log/LogEngine.h>
#include <models/log/SignalSerie.h>
#include <models/log/SignalTick.h>
#include <panes/GraphListPane.h>
#include <models/graphs/GraphView.h>
#include <models/script/ScriptingEngine.h>

///////////////////////////////////////////////////////////////////////////////////
//// OVERRIDES ////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////

bool LogPane::Init() {
    return true;
}

void LogPane::Unit() {}

bool LogPane::DrawPanes(const uint32_t& /*vCurrentFrame*/, bool* vOpened, ImGuiContext* vContextPtr, void* /*vUserDatas*/) {
    ImGui::SetCurrentContext(vContextPtr);
    bool change = false;
    if (vOpened != nullptr && *vOpened) {
        static ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_MenuBar;
        if (ImGui::Begin(GetName().c_str(), vOpened, flags)) {
#ifdef USE_DECORATIONS_FOR_RESIZE_CHILD_WINDOWS
            auto win = ImGui::GetCurrentWindowRead();
            if (win->Viewport->Idx != 0)
                flags |= ImGuiWindowFlags_NoResize;  // | ImGuiWindowFlags_NoTitleBar;
            else
                flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_MenuBar;
#endif
            if (ProjectFile::Instance()->IsProjectLoaded()) {
                if (ImGui::BeginMenuBar()) {
                    DrawMenuBar();
                    ImGui::EndMenuBar();
                }
                DrawTable();
            }
        }

        // MainFrame::sAnyWindowsHovered |= ImGui::IsWindowHovered();

        ImGui::End();
    }
    return change;
}

void LogPane::Clear() {
    m_LogDatas.clear();
}

void LogPane::CheckItem(SignalTickPtr vSignalTick) {
    if (vSignalTick && ImGui::IsItemHovered()) {
        LogEngine::Instance()->SetHoveredTime(vSignalTick->time_epoch);

        if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
            LogEngine::Instance()->ShowHideSignal(vSignalTick->category, vSignalTick->name);
            ProjectFile::Instance()->SetProjectChange();
            ToolPane::Instance()->UpdateTree();
            GraphListPane::Instance()->UpdateDB();

            m_need_re_preparation = true;
        }

        // first mark
        if (ImGui::IsKeyPressed(ImGuiKey_F)) {
            LogEngine::Instance()->SetFirstDiffMark(vSignalTick->time_epoch);
        }

        // second mark
        if (ImGui::IsKeyPressed(ImGuiKey_S)) {
            LogEngine::Instance()->SetSecondDiffMark(vSignalTick->time_epoch);
        }

        // second mark
        if (ImGui::IsKeyPressed(ImGuiKey_R)) {
            LogEngine::Instance()->SetFirstDiffMark(0.0);
            LogEngine::Instance()->SetSecondDiffMark(0.0);
        }
    }
}

void LogPane::DrawMenuBar() {
    bool need_update = false;
    if (ImGui::BeginMenu("Settings")) {
        if (ImGui::MenuItem("Show variable signals only", nullptr, &ProjectFile::Instance()->m_ShowVariableSignalsInLogView)) {
            LogEngine::Instance()->SetHoveredTime(LogEngine::Instance()->GetHoveredTime());
            need_update = true;
        }
        if (ImGui::MenuItem("Collapse Selection", nullptr, &ProjectFile::Instance()->m_CollapseLogSelection)) {
            need_update = true;
        }
        if (ImGui::MenuItem("Hide some values", nullptr, &ProjectFile::Instance()->m_HideSomeLogValues)) {
            need_update = true;
        }
        ImGui::EndMenu();
    }

    if (ProjectFile::Instance()->m_HideSomeLogValues) {
        ImGui::Text("(?)");
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", "you can define many values, ex : 1,2,3.2,5.8");
        }

        if (ImGui::ContrastedButton("R##ResetLogPaneTable")) {
            ProjectFile::Instance()->m_LogValuesToHide.clear();
            need_update = true;
        }

        static char _values_hide_buffer[1024 + 1] = "";
        snprintf(_values_hide_buffer, 1024, "%s", ProjectFile::Instance()->m_LogValuesToHide.c_str());
        if (ImGui::InputText("##Valuestohide", _values_hide_buffer, 1024)) {
            need_update = true;
            ProjectFile::Instance()->m_LogValuesToHide = _values_hide_buffer;
        }
    }

    if (need_update) {
        PrepareLog();
        ProjectFile::Instance()->SetProjectChange();
    }
}

void LogPane::DrawTable() {

    static ImGuiTableFlags flags = ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_RowBg | ImGuiTableFlags_Hideable | ImGuiTableFlags_ScrollY |
        ImGuiTableFlags_Resizable |
        ImGuiTableFlags_NoHostExtendY;

    // first display
    if (m_LogDatas.empty()) {
        PrepareLog();
    }

    const auto _count_logs = m_LogDatas.size();

    m_need_re_preparation = false;

    auto listViewID = ImGui::GetID("##LogPane_DrawTable");
    if (ImGui::BeginTableEx("##LogPane_DrawTable", listViewID, 5, flags))  //-V112
    {
        ImGui::TableSetupScrollFreeze(0, 1);  // Make header always visible
        ImGui::TableSetupColumn("Epoch", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_DefaultHide);
        ImGui::TableSetupColumn("Date", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("Cat", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthFixed);

        ImGui::TableNextRow(ImGuiTableRowFlags_Headers);

        for (int column = 0; column < 5; column++)  //-V112
        {
            ImGui::TableSetColumnIndex(column);
            const char* column_name = ImGui::TableGetColumnName(column);  // Retrieve name passed to TableSetupColumn()
            ImGui::PushID(column);
            ImGui::TableHeader(column_name);
            ImGui::PopID();
        }

        uint32_t count_color_push = 0U;
        ImU32 color = 0U;
        bool selected = false;
        m_LogListClipper.Begin((int)_count_logs, ImGui::GetTextLineHeightWithSpacing());
        while (m_LogListClipper.Step()) {
            for (int i = m_LogListClipper.DisplayStart; i < m_LogListClipper.DisplayEnd; ++i) {
                if (i < 0)
                    continue;

                const auto infos_ptr = m_LogDatas.at((size_t)i).lock();
                if (infos_ptr) {
                    ImGui::TableNextRow();

                    selected = LogEngine::Instance()->isSignalShown(infos_ptr->category, infos_ptr->name, &color);
                    if (selected && color) {
                        ImGui::PushStyleColor(ImGuiCol_Header, (ImU32)color);
                        ImGui::PushStyleColor(ImGuiCol_HeaderActive, (ImU32)color);
                        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, (ImU32)color);
                        count_color_push = 3U;
                        if (ImGui::PushStyleColorWithContrast1(
                                ImGuiCol_Header, ImGuiCol_Text, ImGui::CustomStyle::puContrastedTextColor, ImGui::CustomStyle::puContrastRatio)) {
                            count_color_push = 4U;
                        }
                    } else {
                        color = 0U;
                    }

                    if (ImGui::TableNextColumn())  // time
                    {
                        ImGui::Selectable(ez::str::toStr("%f", infos_ptr->time_epoch).c_str(), &selected, ImGuiSelectableFlags_SpanAllColumns);
                        CheckItem(infos_ptr);
                    }
                    if (ImGui::TableNextColumn())  // date time
                    {
                        ImGui::Selectable(infos_ptr->time_date_time.c_str(), &selected, ImGuiSelectableFlags_SpanAllColumns);
                        CheckItem(infos_ptr);
                    }
                    if (ImGui::TableNextColumn())  // category
                    {
                        ImGui::Selectable(infos_ptr->category.c_str(), &selected, ImGuiSelectableFlags_SpanAllColumns);
                        CheckItem(infos_ptr);
                    }
                    if (ImGui::TableNextColumn())  // name
                    {
                        ImGui::Selectable(infos_ptr->name.c_str(), &selected, ImGuiSelectableFlags_SpanAllColumns);
                        CheckItem(infos_ptr);
                    }
                    if (ImGui::TableNextColumn())  // value
                    {
                        if (infos_ptr->string.empty()) {
                            ImGui::Text("%f", infos_ptr->value);
                        } else {
                            if (infos_ptr->status == LogEngine::sc_START_ZONE) {
                                ImGui::Text(ICON_FONT_ARROW_RIGHT_THICK " %s", infos_ptr->string.c_str());
                            } else if (infos_ptr->status == LogEngine::sc_END_ZONE) {
                                ImGui::Text("%s " ICON_FONT_ARROW_LEFT_THICK, infos_ptr->string.c_str());
                            } else {
                                ImGui::Text("%s", infos_ptr->string.c_str());
                            }
                        }
                        CheckItem(infos_ptr);
                    }

                    if (color) {
                        ImGui::PopStyleColor(count_color_push);
                    }
                }
            }
        }
        m_LogListClipper.End();

        ImGui::EndTable();
    }

    if (m_need_re_preparation) {
        PrepareLog();
    }
}

void LogPane::PrepareLog() {
    if (ScriptingEngine::Instance()->IsJoinable())
        return;

    m_LogDatas.clear();

    if (ProjectFile::Instance()->m_HideSomeLogValues) {
        m_ValuesToHide.clear();
        auto arr = ez::str::splitStringToVector(ProjectFile::Instance()->m_LogValuesToHide, ",");
        for (const auto& a : arr) {
            m_ValuesToHide.push_back(ez::dvariant(a).GetD());
        }
    }

    const auto _count_logs = LogEngine::Instance()->GetSignalTicks().size();
    const auto _collapseSelection = ProjectFile::Instance()->m_CollapseLogSelection;

    for (size_t idx = 0U; idx < _count_logs; ++idx) {
        const auto& infos_ptr = LogEngine::Instance()->GetSignalTicks().at(idx);
        if (infos_ptr) {
            auto parent_ptr = infos_ptr->parent.lock();
            if (parent_ptr != nullptr) {
                if (ProjectFile::Instance()->m_ShowVariableSignalsInLogView && parent_ptr->isConstant()) {
                    continue;
                }
            }

            auto selected = LogEngine::Instance()->isSignalShown(infos_ptr->category, infos_ptr->name);
            if (_collapseSelection && !selected)
                continue;

            if (ProjectFile::Instance()->m_HideSomeLogValues) {
                bool found = false;

                for (const auto& a : m_ValuesToHide) {
                    if (ez::isEqual(a, infos_ptr->value)) {
                        found = true;
                        break;
                    }
                }

                if (found) {
                    continue;
                }
            }

            m_LogDatas.push_back(infos_ptr);
        }
    }
}

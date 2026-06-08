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

#include "LogPaneSecondView.h"
#include <panes/misc/ToolPane.h>
#include <panes/graph/GraphListPane.h>
#include <project/ProjectFile.h>
#include <cinttypes>  // printf zu

#include <models/log/LogEngine.h>
#include <models/log/SignalSerie.h>
#include <models/log/SignalTick.h>
#include <models/script/ScriptingEngine.h>

#include <ezlibs/ezCsv.hpp>

///////////////////////////////////////////////////////////////////////////////////
//// OVERRIDES ////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////

bool LogPaneSecondView::init()  {
    return true;
}

void LogPaneSecondView::unit() {}

bool LogPaneSecondView::drawPanes(bool* apOpened, LayoutPaneUserDatas apUserDatas) {
    
    bool change = false;
    if (apOpened != nullptr && *apOpened) {
        static ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_MenuBar;
        if (ImGui::Begin(getName().c_str(), apOpened, flags)) {
#ifdef USE_DECORATIONS_FOR_RESIZE_CHILD_WINDOWS
            auto win = ImGui::GetCurrentWindowRead();
            if (win->Viewport->Idx != 0)
                flags |= ImGuiWindowFlags_NoResize;  // | ImGuiWindowFlags_NoTitleBar;
            else
                flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_MenuBar;
#endif
            if (ProjectFile::ref()->IsProjectLoaded()) {
                if (ImGui::BeginMenuBar()) {
                    m_drawMenuBar();
                    ImGui::EndMenuBar();
                }
                m_drawTable();
            }
        }

        // MainFrame::sAnyWindowsHovered |= ImGui::IsWindowHovered();

        ImGui::End();
    }
    return change;
}

bool LogPaneSecondView::drawDialogsAndPopups(const ImRect& aRect, LayoutPaneUserDatas apUserDatas) {
    if (ImGuiFileDialog::ref().Display("EXPORT_TO_CSV")) {
        if (ImGuiFileDialog::ref().IsOk()) {
            m_exportToCSV(ImGuiFileDialog::ref().GetFilePathName());
        }
        ImGuiFileDialog::ref().Close();
    }
    return false;
}

void LogPaneSecondView::Clear() {
    m_LogDatas.clear();
}

void LogPaneSecondView::CheckItem(const SignalTickPtr& vSignalTick) {
    if (vSignalTick && ImGui::IsItemHovered()) {
        LogEngine::ref()->SetHoveredTime(vSignalTick->time_epoch);

        if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
            LogEngine::ref()->ShowHideSignal(vSignalTick->category, vSignalTick->name);
            ProjectFile::ref()->SetProjectChange();
            ToolPane::ref()->UpdateTree();
            GraphListPane::ref()->UpdateDB();

            m_need_re_preparation = true;
        }

        // first mark
        if (ImGui::IsKeyPressed(ImGuiKey_F)) {
            LogEngine::ref()->SetFirstDiffMark(vSignalTick->time_epoch);
        }

        // second mark
        if (ImGui::IsKeyPressed(ImGuiKey_S)) {
            LogEngine::ref()->SetSecondDiffMark(vSignalTick->time_epoch);
        }

        // second mark
        if (ImGui::IsKeyPressed(ImGuiKey_R)) {
            LogEngine::ref()->SetFirstDiffMark(0.0);
            LogEngine::ref()->SetSecondDiffMark(0.0);
        }
    }
}

void LogPaneSecondView::m_drawMenuBar() {
    bool need_update = false;
    if (ImGui::BeginMenu("Settings")) {
        if (ImGui::MenuItem("Collapse Selection", nullptr, &ProjectFile::ref()->m_CollapseLog2ndSelection)) {
            need_update = true;
        }
        if (ImGui::MenuItem("Auto resize columns", nullptr, &ProjectFile::ref()->m_AutoResizeLog2ndColumns)) {
            need_update = true;
        }
        if (ImGui::MenuItem("Show variable signals only", nullptr, &ProjectFile::ref()->m_ShowVariableSignalsInLog2ndView)) {
            LogEngine::ref()->SetHoveredTime(LogEngine::ref()->GetHoveredTime());
            need_update = true;
        }
        if (ImGui::MenuItem("Hide some values", nullptr, &ProjectFile::ref()->m_HideSomeLog2ndValues)) {
            need_update = true;
        }
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Export")) {
        if (ImGui::MenuItem("CSV")) {
            IGFD::FileDialogConfig config;
            config.flags = ImGuiFileDialogFlags_Modal;
            ImGuiFileDialog::ref().OpenDialog("EXPORT_TO_CSV", "Export to CSV", ".csv", config);
        }
        ImGui::EndMenu();
    }

    if (LogEngine::ref()->isSomeSelection()) {
        if (!ProjectFile::ref()->m_CollapseLogSelection) {
            if (m_LogListClipper.DisplayStart > 0) {
                if (ImGui::MenuItem(ICON_FONT_ARROW_UP_THICK)) {
                    m_backSelectionNeeded = true;
                }
            }
            if (m_LogListClipper.DisplayEnd < (static_cast<int32_t>(m_LogDatas.size()) - 1)) {
                if (ImGui::MenuItem(ICON_FONT_ARROW_DOWN_THICK)) {
                    m_nextSelectionNeeded = true;
                }
            }
        }
    }

    if (ProjectFile::ref()->m_HideSomeLog2ndValues) {
        ImGui::Text("(?)");
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", "you can define many values, ex : 1,2,3.2,5.8");
        }

        if (ImGui::ContrastedButton("R##ResetLogPaneTable")) {
            ProjectFile::ref()->m_Log2ndValuesToHide.clear();
            need_update = true;
        }

        static char _values_hide_buffer[1024 + 1] = "";
        snprintf(_values_hide_buffer, 1024, "%s", ProjectFile::ref()->m_Log2ndValuesToHide.c_str());
        if (ImGui::InputText("##Valuestohide", _values_hide_buffer, 1024)) {
            need_update = true;
            ProjectFile::ref()->m_Log2ndValuesToHide = _values_hide_buffer;
        }
    }

    if (need_update) {
        PrepareLog();
        ProjectFile::ref()->SetProjectChange();
    }
}

void LogPaneSecondView::m_goOnNextSelection() {
    int32_t max_idx = m_LogDatas.size();
    for (int32_t idx = m_LogListClipper.DisplayStart + 1; idx < max_idx; ++idx) {
        const auto infos_ptr = m_LogDatas.at(idx).lock();
        if (infos_ptr) {
            if (LogEngine::ref()->isSignalShown(infos_ptr->category, infos_ptr->name)) {
                ImGui::SetScrollY(ImGui::GetScrollY() + ImGui::GetTextLineHeightWithSpacing() * (idx - m_LogListClipper.DisplayStart));
                break;
            }
        }
    }
}

void LogPaneSecondView::m_goOnBackSelection() {
    int32_t max_idx = m_LogDatas.size();
    for (int32_t idx = m_LogListClipper.DisplayStart - 1; idx >= 0; --idx) {
        const auto infos_ptr = m_LogDatas.at(idx).lock();
        if (infos_ptr) {
            if (LogEngine::ref()->isSignalShown(infos_ptr->category, infos_ptr->name)) {
                ImGui::SetScrollY(ImGui::GetScrollY() + ImGui::GetTextLineHeightWithSpacing() * (idx - m_LogListClipper.DisplayStart));
                break;
            }
        }
    }
}

void LogPaneSecondView::m_drawTable() {
    ImGuiTableFlags flags = ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_RowBg | ImGuiTableFlags_Hideable | ImGuiTableFlags_ScrollY |
        ImGuiTableFlags_NoHostExtendY;

    if (!ProjectFile::ref()->m_AutoResizeLog2ndColumns) {
        flags |= ImGuiTableFlags_Resizable;
    }
    
    // first display
    if (m_LogDatas.empty()) {
        PrepareLog();
    }

    const auto _count_logs = m_LogDatas.size();

    m_need_re_preparation = false;

    auto listViewID = ImGui::GetID("##LogPaneSecondView_DrawTable");
    if (ImGui::BeginTableEx("##LogPaneSecondView_DrawTable", listViewID, 5, flags))  //-V112
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

        int32_t count_color_push = 0U;
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

                    selected = LogEngine::ref()->isSignalShown(infos_ptr->category, infos_ptr->name, &color);
                    if (selected && color) {
                        ImGui::PushStyleColor(ImGuiCol_Header, (ImU32)color);
                        ImGui::PushStyleColor(ImGuiCol_HeaderActive, (ImU32)color);
                        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, (ImU32)color);
                        count_color_push = 3;
                        if (ImGui::PushStyleColorWithContrast1(
                                ImGuiCol_Header, ImGuiCol_Text, ImGui::CustomStyle::puContrastedTextColor, ImGui::CustomStyle::puContrastRatio)) {
                            count_color_push = 4;
                        }
                    } else {
                        color = 0U;
                    }

                    if (m_nextSelectionNeeded) {
                        m_nextSelectionNeeded = false;
                        m_goOnNextSelection();
                    }

                    if (m_backSelectionNeeded) {
                        m_backSelectionNeeded = false;
                        m_goOnBackSelection();
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

void LogPaneSecondView::PrepareLog() {
    if (ScriptingEngine::ref()->IsJoinable())
        return;

    m_LogDatas.clear();

    if (ProjectFile::ref()->m_HideSomeLog2ndValues) {
        m_ValuesToHide.clear();
        auto arr = ez::str::splitStringToVector(ProjectFile::ref()->m_Log2ndValuesToHide, ",");
        for (const auto& a : arr) {
            m_ValuesToHide.push_back(ez::dvariant(a).GetD());
        }
    }

    const auto _count_logs = LogEngine::ref()->GetSignalTicks().size();
    const auto _collapseSelection = ProjectFile::ref()->m_CollapseLog2ndSelection;

    for (size_t idx = 0U; idx < _count_logs; ++idx) {
        const auto& infos_ptr = LogEngine::ref()->GetSignalTicks().at(idx);
        if (infos_ptr) {
            auto parent_ptr = infos_ptr->parent.lock();
            if (parent_ptr != nullptr) {
                if (ProjectFile::ref()->m_ShowVariableSignalsInLog2ndView && parent_ptr->isConstant()) {
                    continue;
                }
            }

            auto selected = LogEngine::ref()->isSignalShown(infos_ptr->category, infos_ptr->name);
            if (_collapseSelection && !selected)
                continue;

            if (ProjectFile::ref()->m_HideSomeLog2ndValues) {
                bool found = false;

                for (const auto& a : m_ValuesToHide) {
                    if (ez::math::isEqual(a, infos_ptr->value)) {
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

void LogPaneSecondView::m_exportToCSV(const std::string& aFilePathName) {
    ez::Csv csv_file;

    csv_file.setHeader({
     "time_epoch",
     "time_date_time",
     "category",
     "name",
     "value",
     "status",
     "description" });

    for (const auto& datas : m_LogDatas) {
        auto ptr = datas.lock();
        if (ptr != nullptr) {
            auto str = ptr->string;
            if (str.empty()) {
                str = std::to_string(ptr->value);
            }
            csv_file.appendRow({ std::to_string(ptr->time_epoch), ptr->time_date_time, ptr->category, ptr->name, str, ptr->status, ptr->desc });
        }
    }

    csv_file.writeToFile(aFilePathName, ';');
}

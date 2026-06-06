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

#include "ToolPane.h"
#include <project/ProjectFile.h>
#include <cinttypes>  // printf zu
#include <panes/log/LogPane.h>
#include <panes/misc/CodePane.h>

#include <models/script/ScriptingEngine.h>
#include <models/log/LogEngine.h>
#include <models/log/SourceFile.h>
#include <models/log/SignalSerie.h>
#include <models/log/SignalTick.h>
#include <models/graphs/GraphView.h>

#include <ezlibs/ezFile.hpp>

///////////////////////////////////////////////////////////////////////////////////
//// IMGUI PANE ///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////

void ToolPane::Clear() {
    m_SignalTree.clear();
}

bool ToolPane::init()  {
    return true;
}

void ToolPane::unit() {}

bool ToolPane::drawPanes(bool* apOpened, LayoutPaneUserDatas apUserDatas) {
    
    bool change = false;
    if (apOpened != nullptr && *apOpened) {
        static ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_MenuBar;
        if (ImGui::Begin(getName().c_str(), apOpened, flags)) {
#ifdef USE_DECORATIONS_FOR_RESIZE_CHILD_WINDOWS
            auto win = ImGui::GetCurrentWindowRead();
            if (win->Viewport->Idx != 0)
                flags |= ImGuiWindowFlags_NoResize;
            else
                flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus;
#endif
            if (ProjectFile::ref()->IsProjectLoaded()) {
                DrawTable();
                DrawTree();
            }
        }

        ImGui::End();
    }
    return change;
}

bool ToolPane::drawDialogsAndPopups(const ImRect& aRect, LayoutPaneUserDatas apUserDatas) {
    (void)apUserDatas;
    if (ProjectFile::ref()->IsProjectLoaded()) {
        ImVec2 maxSize = aRect.GetSize();
        ImVec2 minSize = maxSize * 0.5f;

        if (ImGuiFileDialog::ref().Display("OPEN_LOG_FILE", ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking, minSize, maxSize)) {
            if (ImGuiFileDialog::ref().IsOk()) {
                ProjectFile::ref()->m_LastLogFilePath = ImGuiFileDialog::ref().GetFilePathName();
                auto files = ImGuiFileDialog::ref().GetSelection();
                for (const auto& item : files) {
                    ProjectFile::ref()->AddSourceFilePathName(item.second);
                }
                ProjectFile::ref()->SetProjectChange();
            }
            ImGuiFileDialog::ref().Close();
        }

        if (ImGuiFileDialog::ref().Display("EDIT_LOG_FILE", ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking, minSize, maxSize)) {
            if (ImGuiFileDialog::ref().IsOk()) {
                const auto& container = ProjectFile::ref()->GetSourceFilePathNames();
                if (m_CurrentSourceEdited > -1 && m_CurrentSourceEdited < (int32_t)container.size()) {
                    auto fpn = ImGuiFileDialog::ref().GetFilePathName();
                    auto ps = ez::file::parsePathFileName(fpn);
                    if (ps.isOk) {
                        ProjectFile::ref()->RemoveFilePathName(container[m_CurrentSourceEdited].second);
                        ProjectFile::ref()->AddSourceFilePathName(fpn);
                        ProjectFile::ref()->SetProjectChange();
                    }
                }
            }
            ImGuiFileDialog::ref().Close();
        }
    }
    return false;
}

void ToolPane::UpdateTree() {
    m_SignalTree.prepare(ProjectFile::ref()->m_SearchString);
}

void ToolPane::DrawTable() {
    // the script is no longer an external file: it is imported via the menu bar
    // (Project > Import script), edited in the Code pane and stored in the .ltg db.
    if (ImGui::CollapsingHeader("Log Files")) {
        if (ImGui::ContrastedButton("Add a Log File", nullptr, nullptr, -1.0f, ImVec2(-1.0f, 0.0f))) {
            IGFD::FileDialogConfig config;
            config.countSelectionMax = 1;
            config.filePathName = ProjectFile::ref()->m_LastLogFilePath;
            config.flags = ImGuiFileDialogFlags_Modal;
            ImGuiFileDialog::ref().OpenDialog("OPEN_LOG_FILE", "Open a Log File", ".*", config);
        }

        const auto& sources = ProjectFile::ref()->GetSourceFilePathNames();
        if (!sources.empty()) {
            static ImGuiTableFlags flags = ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders;
            if (ImGui::BeginTable("##sourcefilestable", 3, flags, ImVec2(-1.0f, sources.size() * ImGui::GetTextLineHeightWithSpacing()))) {
                ImGui::TableSetupScrollFreeze(0, 1);  // Make header always visible
                ImGui::TableSetupColumn("##Edit", ImGuiTableColumnFlags_WidthFixed);
                ImGui::TableSetupColumn("Log Files", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("##Close", ImGuiTableColumnFlags_WidthFixed);
                ImGui::TableHeadersRow();

                int32_t idx = 0;
                auto it_to_edit = sources.end();
                auto it_to_erase = sources.end();
                for (auto it_source_file = sources.begin(); it_source_file != sources.end(); ++it_source_file) {
                    ImGui::TableNextRow();

                    if (ImGui::TableSetColumnIndex(0)) 
                    {
                        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 1));
                        if (ImGui::ContrastedButton(ICON_FONT_PENCIL "##SourceFileEdit", nullptr, nullptr, 0.0f, ImVec2(16.0f, 16.0f))) {
                            it_to_edit = it_source_file;
                            m_CurrentSourceEdited = idx;
                        }
                        ImGui::PopStyleVar();
                    }

                    if (ImGui::TableSetColumnIndex(1)) 
                    {
                        ImGui::Selectable(it_source_file->first.c_str());
                        if (ImGui::IsItemHovered()) {
                            ImGui::SetTooltip("%s", it_source_file->second.c_str());
                        }
                    }

                    if (ImGui::TableSetColumnIndex(2))
                    {
                        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 1));
                        if (ImGui::ContrastedButton(ICON_FONT_CLOSE "##SourceFileDelete", nullptr, nullptr, 0.0f, ImVec2(16.0f, 16.0f))) {
                            it_to_erase = it_source_file;
                        }
                        ImGui::PopStyleVar();
                    }

                    ++idx;
                }

                // edit
                if (it_to_edit != sources.end()) {
                    IGFD::FileDialogConfig config;
                    config.countSelectionMax = 1;
                    config.filePathName = it_to_edit->second;
                    config.flags = ImGuiFileDialogFlags_Modal;
                    ImGuiFileDialog::ref().OpenDialog("EDIT_LOG_FILE", "Edit a Log File", ".*", config);
                }

                // erase
                if (it_to_erase != sources.end()) {
                    ProjectFile::ref()->RemoveFilePathName(it_to_erase->second);
                }

                ImGui::EndTable();
            }
        }
    }

    if (ImGui::CollapsingHeader("Predefined Zero value")) {
        ImGui::CheckBoxBoolDefault("Use Predefined Zero Value ?", &ProjectFile::ref()->m_UsePredefinedZeroValue, false);
        if (ProjectFile::ref()->m_UsePredefinedZeroValue) {
            ImGui::InputDouble("##Predefinedzerovalue", &ProjectFile::ref()->m_PredefinedZeroValue);
        }
    }

    if (ImGui::CollapsingHeader("Analyse")) {
        ScriptingEngine::ref()->drawMenu();
        if (ScriptingEngine::ref()->isValidScriptingSelected()) {
            if (!ScriptingEngine::ref()->IsJoinable()) {
                if (ImGui::ContrastedButton("Start Analyse of file(s)", nullptr, nullptr, -1.0f, ImVec2(-1.0f, 0.0f))) {
                    ScriptingEngine::ref()->Clear();
                    ScriptingEngine::ref()->SetScriptCode(CodePane::ref()->GetScriptCode());
                    const auto& sources = ProjectFile::ref()->GetSourceFilePathNames();
                    for (const auto& source : sources) {
                        ScriptingEngine::ref()->AddSourceFilePathName(source.second);
                    }
                    ScriptingEngine::ref()->StartWorkerThread(false);
                }
            } else {
                if (ImGui::ContrastedButton("Stop Analyse", nullptr, nullptr, -1.0f, ImVec2(-1.0f, 0.0f))) {
                    ScriptingEngine::ref()->StopWorkerThread();
                }

                auto progress = (float)ScriptingEngine::s_progress;
                ImGui::ProgressBar(progress);
            }
        }
    }
}

void ToolPane::DrawTree() {
    auto& search_string = ProjectFile::ref()->m_SearchString;

    ImGui::Header("Signals");

    bool _collapse_all = false;
    bool _expand_all = false;

    const float fw = ImGui::GetContentRegionAvail().x;

    const float aw = (fw - ImGui::GetStyle().ItemSpacing.x) * 0.5f;
    if (ImGui::ContrastedButton("Collapse All##ToolPane_DrawTree", nullptr, nullptr, aw)) {
        _collapse_all = true;
    }

    ImGui::SameLine();

    if (ImGui::ContrastedButton("Expand All##ToolPane_DrawTree", nullptr, nullptr, aw)) {
        _expand_all = true;
    }

    if (ImGui::ContrastedButton("Hide All Graphs##ToolPane_DrawTree", nullptr, nullptr, fw)) {
        HideAllGraphs();
    }

    ImGui::Text("Search : ");
    ImGui::SameLine();

    snprintf(m_search_buffer, 1024, "%s", search_string.c_str());
    if (ImGui::ContrastedButton("R##ToolPane_DrawTree")) {
        search_string.clear();
        m_search_buffer[0] = '\0';
        m_SignalTree.prepare(search_string);
    }
    ImGui::SameLine();
    if (ImGui::InputText("##ToolPane_DrawTree_Search", m_search_buffer, 1024)) {
        search_string = ez::str::toLower(m_search_buffer);
        m_SignalTree.prepare(search_string);
    }

    if (ImGui::BeginChild("##Items_ToolPane_DrawTree")) {
        m_SignalTree.displayTree(_collapse_all, _expand_all);
    }
    ImGui::EndChild();
}

void ToolPane::HideAllGraphs() {
    bool _one_at_least = false;

    for (auto& item_cat : LogEngine::ref()->GetSignalSeries()) {
        for (auto& item_name : item_cat.second) {
            if (item_name.second) {
                if (item_name.second->show) {
                    _one_at_least = true;
                }

                LogEngine::ref()->ShowHideSignal(item_name.second->category, item_name.second->name, false);
            }
        }
    }

    if (_one_at_least) {
        GraphView::ref()->Clear();
        ProjectFile::ref()->SetProjectChange();
    }
}

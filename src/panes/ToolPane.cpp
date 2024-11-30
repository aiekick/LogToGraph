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
#include <panes/LogPane.h>
#include <panes/CodePane.h>

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

bool ToolPane::Init() {
    return true;
}

void ToolPane::Unit() {}

bool ToolPane::DrawPanes(const uint32_t& /*vCurrentFrame*/, bool* vOpened, ImGuiContext* vContextPtr, void* /*vUserDatas*/) {
    ImGui::SetCurrentContext(vContextPtr);
    bool change = false;
    if (vOpened != nullptr && *vOpened) {
        static ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_MenuBar;
        if (ImGui::Begin(GetName().c_str(), vOpened, flags)) {
#ifdef USE_DECORATIONS_FOR_RESIZE_CHILD_WINDOWS
            auto win = ImGui::GetCurrentWindowRead();
            if (win->Viewport->Idx != 0)
                flags |= ImGuiWindowFlags_NoResize;
            else
                flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus;
#endif
            if (ProjectFile::Instance()->IsProjectLoaded()) {
                DrawTable();

                DrawTree();
            }
        }

        ImGui::End();
    }
    return change;
}

bool ToolPane::DrawDialogsAndPopups(const uint32_t& /*vCurrentFrame*/, const ImRect& vRect, ImGuiContext* /*vContextPtr*/, void* /*vUserDatas*/) {
    if (ProjectFile::Instance()->IsProjectLoaded()) {
        ImVec2 maxSize = vRect.GetSize();
        ImVec2 minSize = maxSize * 0.5f;

        if (ImGuiFileDialog::Instance()->Display("OPEN_LUA_SCRIPT_FILE", ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking, minSize, maxSize)) {
            if (ImGuiFileDialog::Instance()->IsOk()) {
                ProjectFile::Instance()->SetScriptFilePathName(ImGuiFileDialog::Instance()->GetFilePathName());
                CodePane::Instance()->OpenFile(ImGuiFileDialog::Instance()->GetFilePathName());
                ProjectFile::Instance()->SetProjectChange();
            }

            ImGuiFileDialog::Instance()->Close();
        }

        if (ImGuiFileDialog::Instance()->Display("OPEN_LOG_FILE", ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking, minSize, maxSize)) {
            if (ImGuiFileDialog::Instance()->IsOk()) {
                ProjectFile::Instance()->m_LastLogFilePath = ImGuiFileDialog::Instance()->GetFilePathName();
                auto files = ImGuiFileDialog::Instance()->GetSelection();
                for (const auto& item : files) {
                    ProjectFile::Instance()->AddSourceFilePathName(item.second);
                }

                ProjectFile::Instance()->SetProjectChange();
            }

            ImGuiFileDialog::Instance()->Close();
        }

        if (ImGuiFileDialog::Instance()->Display("EDIT_LOG_FILE", ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking, minSize, maxSize)) {
            if (ImGuiFileDialog::Instance()->IsOk()) {
                const auto& container = ProjectFile::Instance()->GetSourceFilePathNames();
                if (m_CurrentSourceEdited > -1 && m_CurrentSourceEdited < (int32_t)container.size()) {
                    auto fpn = ImGuiFileDialog::Instance()->GetFilePathName();
                    auto ps = ez::file::parsePathFileName(fpn);
                    if (ps.isOk) {
                        ProjectFile::Instance()->RemoveFilePathName(container[m_CurrentSourceEdited].second);
                        ProjectFile::Instance()->AddSourceFilePathName(fpn);
                        ProjectFile::Instance()->SetProjectChange();
                    }
                }
            }

            ImGuiFileDialog::Instance()->Close();
        }
    }
    return false;
}

void ToolPane::UpdateTree() {
    m_SignalTree.prepare(ProjectFile::Instance()->m_SearchString);
}

void ToolPane::DrawTable() {
    if (ImGui::CollapsingHeader("Script Script File")) {
        if (ImGui::ContrastedButton("Select the Script Script File", nullptr, nullptr, -1.0f, ImVec2(-1.0f, 0.0f))) {
            IGFD::FileDialogConfig config;
            config.countSelectionMax = 1;
            config.filePathName = ProjectFile::Instance()->GetScriptFilePathName();
            config.flags = ImGuiFileDialogFlags_Modal;
            ImGuiFileDialog::Instance()->OpenDialog("OPEN_LUA_SCRIPT_FILE", "Open a Script Script File", ".lua,.*", config);
        }
        if (ImGui::ContrastedButton(ICON_FONT_PENCIL "##ScriptScriptEdit")) {
            ez::file::openFile(ProjectFile::Instance()->GetScriptFilePathName());
        }
        ImGui::SameLine();
        ImGui::TextWrapped("%s", ProjectFile::Instance()->GetScriptFileName().c_str());
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", ProjectFile::Instance()->GetScriptFilePathName().c_str());
        }
    }

    if (ImGui::CollapsingHeader("Log Files")) {
        if (ImGui::ContrastedButton("Add a Log File", nullptr, nullptr, -1.0f, ImVec2(-1.0f, 0.0f))) {
            IGFD::FileDialogConfig config;
            config.countSelectionMax = 1;
            config.filePathName = ProjectFile::Instance()->m_LastLogFilePath;
            config.flags = ImGuiFileDialogFlags_Modal;
            ImGuiFileDialog::Instance()->OpenDialog("OPEN_LOG_FILE", "Open a Log File", ".*", config);
        }

        const auto& sources = ProjectFile::Instance()->GetSourceFilePathNames();
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
                    ImGuiFileDialog::Instance()->OpenDialog("EDIT_LOG_FILE", "Edit a Log File", ".*", config);
                }

                // erase
                if (it_to_erase != sources.end()) {
                    ProjectFile::Instance()->RemoveFilePathName(it_to_erase->second);
                }

                ImGui::EndTable();
            }
        }
    }

    if (ImGui::CollapsingHeader("Predefined Zero value")) {
        ImGui::CheckBoxBoolDefault("Use Predefined Zero Value ?", &ProjectFile::Instance()->m_UsePredefinedZeroValue, false);
        if (ProjectFile::Instance()->m_UsePredefinedZeroValue) {
            ImGui::InputDouble("##Predefinedzerovalue", &ProjectFile::Instance()->m_PredefinedZeroValue);
        }
    }

    if (ImGui::CollapsingHeader("Analyse")) {
        ScriptingEngine::Instance()->drawMenu();
        if (ScriptingEngine::Instance()->isValidScriptingSelected()) {
            if (!ScriptingEngine::Instance()->IsJoinable()) {
                if (ImGui::ContrastedButton("Start Analyse of file(s)", nullptr, nullptr, -1.0f, ImVec2(-1.0f, 0.0f))) {
                    ScriptingEngine::Instance()->Clear();
                    ScriptingEngine::Instance()->SetScriptFilePathName(ProjectFile::Instance()->GetScriptFilePathName());
                    const auto& sources = ProjectFile::Instance()->GetSourceFilePathNames();
                    for (const auto& source : sources) {
                        ScriptingEngine::Instance()->AddSourceFilePathName(source.second);
                    }
                    ScriptingEngine::Instance()->StartWorkerThread(false);
                }
            } else {
                if (ImGui::ContrastedButton("Stop Analyse", nullptr, nullptr, -1.0f, ImVec2(-1.0f, 0.0f))) {
                    ScriptingEngine::Instance()->StopWorkerThread();
                }

                auto progress = (float)ScriptingEngine::s_progress;
                ImGui::ProgressBar(progress);
            }
        }
    }
}

void ToolPane::DrawTree() {
    auto& search_string = ProjectFile::Instance()->m_SearchString;

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

    for (auto& item_cat : LogEngine::Instance()->GetSignalSeries()) {
        for (auto& item_name : item_cat.second) {
            if (item_name.second) {
                if (item_name.second->show) {
                    _one_at_least = true;
                }

                LogEngine::Instance()->ShowHideSignal(item_name.second->category, item_name.second->name, false);
            }
        }
    }

    if (_one_at_least) {
        GraphView::Instance()->Clear();
        ProjectFile::Instance()->SetProjectChange();
    }
}

// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/*
 * Copyright 2022-2023 Stephane Cuillerdier (aka Aiekick)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "ProjectFile.h"

#include <models/log/LogEngine.h>
#include <panes/misc/CodePane.h>
#include <models/database/DataBase.h>
#include <models/graphs/GraphView.h>
#include <models/graphs/GraphGroup.h>
#include <panes/log/LogPane.h>
#include <panes/misc/ToolPane.h>
#include <panes/log/LogPaneSecondView.h>
#include <panes/graph/GraphListPane.h>
#include <panes/graph/GraphGroupPane.h>
#include <panes/signals/SignalsHoveredDiff.h>
#include <panes/signals/SignalsHoveredList.h>
#include <panes/signals/SignalsHoveredMap.h>
#include <panes/debug/StackTreePane.h>
#include <panes/debug/ScopePane.h>
#include <panes/debug/CalltracePane.h>
#include <panes/debug/WatcherPane.h>
#include <models/script/ScriptingEngine.h>
#include <models/debug/ScriptDebugger.h>

#include <panes/graph/GraphPane.h>

#include <settings/SettingsDialog.h>

#include <ezlibs/ezFile.hpp>

ProjectFile::ProjectFile() = default;

ProjectFile::ProjectFile(const std::string& vFilePathName) {
    m_ProjectFilePathName = ez::file::simplifyFilePath(vFilePathName);
    auto ps = ez::file::parsePathFileName(m_ProjectFilePathName);
    if (ps.isOk) {
        m_ProjectFileName = ps.name;
        m_ProjectFilePath = ps.path;
    }
}

ProjectFile::~ProjectFile() = default;

void ProjectFile::Clear() {
    m_ProjectFilePathName.clear();
    m_ProjectFileName.clear();
    m_ProjectFilePath.clear();
    m_ScriptFilePathName.clear();
    m_ScriptFileName.clear();
    m_SourceFilePathNames.clear();
    m_IsLoaded = false;
    m_IsThereAnyChanges = false;
    Messaging::ref().Clear();
}

void ProjectFile::ClearDatas() {
    // join any in-progress worker (incl. one paused at a breakpoint) before resetting any state it touches
    ScriptingEngine::ref()->AbortAndJoinWorker();
    // debugger state (breakpoints, arm flag, paused snapshot) — wiped before the panes that cache it
    ScriptDebugger::ref()->Clear();
    ScriptingEngine::ref()->Clear();
    LogEngine::ref()->Clear();
    GraphView::ref()->Clear();
    // GraphGroup is no longer a singleton — instances live inside GraphView::m_GraphGroups
    // and are cleared by GraphView::Clear() above
    ToolPane::ref()->Clear();
    LogPane::ref()->Clear();
    LogPaneSecondView::ref()->Clear();
    GraphListPane::ref()->Clear();
    GraphGroupPane::ref()->Clear();
    SignalsHoveredDiff::ref()->Clear();
    SignalsHoveredList::ref()->Clear();
    SignalsHoveredMap::ref()->Clear();
    // code editor (sheets + debug caches) — wipe last so its caches don't repopulate from stale debugger state
    CodePane::ref()->Clear();
    StackTreePane::ref()->Clear();
    ScopePane::ref()->Clear();
    CalltracePane::ref()->Clear();
    WatcherPane::ref()->Clear();
    // per-project settings (forwarded to every registered ISettings; default no-op for APP-only impls)
    SettingsDialog::ref().clearProjectSettings();
}

void ProjectFile::New() {
    Clear();
    ClearDatas();
    m_IsLoaded = true;
    m_NeverSaved = true;
    SetProjectChange(true);
}

void ProjectFile::New(const std::string& vFilePathName) {
    Clear();
    ClearDatas();
    m_ProjectFilePathName = ez::file::simplifyFilePath(vFilePathName);
    DataBase::ref()->CreateDBFile(m_ProjectFilePathName);
    auto ps = ez::file::parsePathFileName(m_ProjectFilePathName);
    if (ps.isOk) {
        m_ProjectFileName = ps.name;
        m_ProjectFilePath = ps.path;
    }
    m_IsLoaded = true;
    SetProjectChange(false);
}

bool ProjectFile::Load() {
    return LoadAs(m_ProjectFilePathName);
}

// ils wanted to not pass the adress for re open case
// elwse, the clear will set vFilePathName to empty because with re open, target m_ProjectFilePathName
bool ProjectFile::LoadAs(const std::string& vFilePathName) {
    if (!vFilePathName.empty()) {
        Clear();
        std::string filePathName = ez::file::simplifyFilePath(vFilePathName);
        if (DataBase::ref()->IsFileASqlite3DB(filePathName)) {
            if (DataBase::ref()->OpenDBFile(filePathName)) {
                ClearDatas();
                auto xml_settings = DataBase::ref()->GetSettingsXMLDatas();
                if (LoadConfigString(ez::xml::Node::unEscapeXml(xml_settings), "") || xml_settings.empty()) {
                    m_ProjectFilePathName = ez::file::simplifyFilePath(vFilePathName);
                    auto ps = ez::file::parsePathFileName(m_ProjectFilePathName);
                    if (ps.isOk) {
                        m_ProjectFileName = ps.name;
                        m_ProjectFilePath = ps.path;
                        auto scriptCode = DataBase::ref()->GetScriptCode();
                        if (scriptCode.empty() && !m_ScriptFilePathName.empty() && ez::file::isFileExist(m_ScriptFilePathName)) {
                            // retrocompat: import the old external script once, then it lives in the db
                            scriptCode = ez::file::loadFileToString(m_ScriptFilePathName);
                        }
                        CodePane::ref()->OpenScript(scriptCode);
                    }
                    m_IsLoaded = true;
                    SetProjectChange(false);
                } else {
                    Clear();
                    LogVarError("The project file %s cant be loaded", filePathName.c_str());
                }

                LogEngine::ref()->Finalize();
                GraphListPane::ref()->UpdateDB();
                ToolPane::ref()->UpdateTree();
                LogEngine::ref()->PrepareAfterLoad();

                DataBase::ref()->CloseDBFile();
            }
        }
    }
    return m_IsLoaded;
}

bool ProjectFile::Save() {
    if (m_NeverSaved) {
        return false;
    }

    LogEngine::ref()->PrepareForSave();
    if (DataBase::ref()->OpenDBFile(m_ProjectFilePathName)) {
        auto xml_settings = ez::xml::Node::escapeXml(SaveConfigString("", "config"));
        if (DataBase::ref()->SetSettingsXMLDatas(xml_settings)) {
            DataBase::ref()->SetScriptCode(CodePane::ref()->GetScriptCode());
            CodePane::ref()->MarkScriptSaved();  // the editor is now in sync with the db
            SetProjectChange(false);
            DataBase::ref()->CloseDBFile();
            return true;
        }
        DataBase::ref()->CloseDBFile();
    }

    return false;
}

bool ProjectFile::SaveAs(const std::string& vFilePathName) {
    std::string filePathName = ez::file::simplifyFilePath(vFilePathName);
    auto ps = ez::file::parsePathFileName(filePathName);
    if (ps.isOk) {
        m_ProjectFilePathName = ps.GetFPNE_WithExt(PROJECT_EXT_DOT_LESS);
        m_ProjectFilePath = ps.path;
        m_NeverSaved = false;
        return Save();
    }
    return false;
}

bool ProjectFile::IsProjectLoaded() const {
    return m_IsLoaded;
}

bool ProjectFile::IsProjectNeverSaved() const {
    return m_NeverSaved;
}

bool ProjectFile::IsThereAnyProjectChanges() const {
    return m_IsThereAnyChanges;
}

void ProjectFile::SetProjectChange(bool vChange) {
    m_IsThereAnyChanges = vChange;
    m_WasJustSaved = true;
    m_WasJustSavedFrameCounter = 2U;
}

void ProjectFile::NewFrame() {
    if (m_WasJustSavedFrameCounter) {
        --m_WasJustSavedFrameCounter;
    } else {
        m_WasJustSaved = false;
    }
}

bool ProjectFile::WasJustSaved() {
    return m_WasJustSaved;
}

std::string ProjectFile::GetProjectFilepathName() const {
    return m_ProjectFilePathName;
}

void ProjectFile::SetScriptFilePathName(const SourceFilePathName& vFilePathName) {
    m_ScriptFilePathName = vFilePathName;
    auto ps = ez::file::parsePathFileName(m_ScriptFilePathName);
    if (ps.isOk) {
        m_ScriptFileName = ps.GetFPNE_WithPath("");
        SetProjectChange(true);
    }
}

const SourceFilePathName& ProjectFile::GetScriptFilePathName() const {
    return m_ScriptFilePathName;
}

const SourceFileName& ProjectFile::GetScriptFileName() const {
    return m_ScriptFileName;
}

void ProjectFile::AddSourceFilePathName(const SourceFilePathName& vFilePathName) {
    auto ps = ez::file::parsePathFileName(vFilePathName);
    if (ps.isOk) {
        m_SourceFilePathNames.emplace_back(ps.GetFPNE_WithPath(""), vFilePathName);
        SetProjectChange(true);
    }
}

bool ProjectFile::RemoveFilePathName(const SourceFilePathName& vFilePathName) {
    for (auto it = m_SourceFilePathNames.begin(); it != m_SourceFilePathNames.end(); ++it) {
        if (it->second == vFilePathName) {
            m_SourceFilePathNames.erase(it);
            return true;
        }
    }
    return false;
}

const std::vector<std::pair<SourceFileName, SourceFilePathName>>& ProjectFile::GetSourceFilePathNames() const {
    return m_SourceFilePathNames;
}

ez::xml::Nodes ProjectFile::getXmlNodes(const std::string& /*vUserDatas*/) {
    ez::xml::Node node;
    node.setName("project");
    node.addChilds(ImLayout::ref().getXmlNodes("project"));
    node.addChilds(ScriptingEngine::ref()->getXmlNodes("project"));
    node.addChilds(LogEngine::ref()->getXmlNodes("project"));
    node.addChilds(SettingsDialog::ref().getXmlNodes("project"));
    node.addChild("graph_bar_colors").setContent(m_GraphColors.graphBarColor);
    node.addChild("graph_bar_colors").setContent(m_GraphColors.graphBarColor);
    node.addChild("graph_current_time_colors").setContent(m_GraphColors.graphHoveredTimeColor);
    node.addChild("graph_mouse_current_time_colors").setContent(m_GraphColors.graphMouseHoveredTimeColor);
    node.addChild("graph_diff_first_mark_color").setContent(m_GraphColors.graphFirstDiffMarkColor);
    node.addChild("graph_diff_second_mark_color").setContent(m_GraphColors.graphSecondDiffMarkColor);
    node.addChild("graph_hovered_updated_rect_color").setContent(m_GraphColors.graphHoveredUpdatedRectColor);
    node.addChild("selection_collapsing").setContent(m_CollapseLogSelection);
    node.addChild("auto_colorize").setContent(m_AutoColorize);
    node.addChild("search_string").setContent(m_SearchString);
    node.addChild("all_graphs_signals_search_string").setContent(m_AllGraphSignalsSearchString);
    node.addChild("log_values_to_hide").setContent(m_LogValuesToHide);
    node.addChild("log_2nd_values_to_hide").setContent(m_Log2ndValuesToHide);
    node.addChild("hide_some_log_values").setContent(m_HideSomeLogValues);
    node.addChild("hide_some_log_2nd_values").setContent(m_HideSomeLog2ndValues);
    node.addChild("signals_preview_count_x").setContent(m_SignalPreview_CountX);
    node.addChild("signals_preview_size_x").setContent(m_SignalPreview_SizeX);
    node.addChild("graph_diff_first_mark").setContent(m_DiffFirstMark);
    node.addChild("graph_diff_second_mark").setContent(m_DiffSecondMark);
    node.addChild("graph_synchronize").setContent(m_SyncGraphs);
    node.addChild("graph_sync_limits").setContent(m_SyncGraphsLimits);
    node.addChild("code_file_path_name").setContent(m_CodeFilePathName);
    node.addChild("curve_radius_detection").setContent(m_CurveRadiusDetection);
    node.addChild("selected_curve_display_thickness").setContent(m_SelectedCurveDisplayThickNess);
    node.addChild("default_curve_display_thickness").setContent(m_DefaultCurveDisplayThickNess);
    node.addChild("use_predefined_zero_value").setContent(m_UsePredefinedZeroValue);
    node.addChild("predefined_zero_value").setContent(m_PredefinedZeroValue);
    node.addChild("show_variable_signals_in_all_graph_view").setContent(m_ShowVariableSignalsInAllGraphView);
    node.addChild("show_variable_signals_in_graph_view").setContent(m_ShowVariableSignalsInGraphView);
    node.addChild("show_variable_signals_in_hovered_list_view").setContent(m_ShowVariableSignalsInHoveredListView);
    node.addChild("show_variable_signals_in_log_view").setContent(m_ShowVariableSignalsInLogView);
    node.addChild("show_variable_signals_in_log_2nd_view").setContent(m_ShowVariableSignalsInLog2ndView);
    node.addChild("auto_resize_columns_log_view").setContent(m_AutoResizeLogColumns);
    node.addChild("auto_resize_columns_log_2nd_view").setContent(m_AutoResizeLog2ndColumns);
    node.addChild("hovered_list_changed_text_rect_thickness").setContent(m_HoveredListChangedTextRectThickNess);
    node.addChild("project_script_font_scale").setContent(m_ProjectScriptFontScale);
    node.addChild("last_log_file_path").setContent(m_LastLogFilePath);
    node.addChild("script_file").setContent(m_ScriptFilePathName);
    auto& childNode = node.addChild("log_files");
    for (const auto& file : m_SourceFilePathNames) {
        childNode.addChild("log_file").setContent(ez::xml::Node::escapeXml(file.second));
    }
    return {node};
}

bool ProjectFile::setFromXmlNodes(const ez::xml::Node& vNode, const ez::xml::Node& vParent, const std::string& vUserDatas) {
    const auto& strName = vNode.getName();
    const auto& strValue = vNode.getContent();
    const auto& strParentName = vParent.getName();

    if (strParentName == "project") {
        if (strName == "graph_bar_colors") {
            m_GraphColors.graphBarColor = ez::fvariant(strValue).GetV4();
        } else if (strName == "graph_current_time_colors") {
            m_GraphColors.graphHoveredTimeColor = ez::fvariant(strValue).GetV4();
        } else if (strName == "graph_mouse_current_time_colors") {
            m_GraphColors.graphMouseHoveredTimeColor = ez::fvariant(strValue).GetV4();
        } else if (strName == "graph_diff_first_mark_color") {
            m_GraphColors.graphFirstDiffMarkColor = ez::fvariant(strValue).GetV4();
        } else if (strName == "graph_diff_second_mark_color") {
            m_GraphColors.graphSecondDiffMarkColor = ez::fvariant(strValue).GetV4();
        } else if (strName == "graph_hovered_updated_rect_color") {
            m_GraphColors.graphHoveredUpdatedRectColor = ez::fvariant(strValue).GetV4();
        } else if (strName == "selection_collapsing") {
            m_CollapseLogSelection = ez::ivariant(strValue).GetB();
        } else if (strName == "auto_colorize") {
            m_AutoColorize = ez::ivariant(strValue).GetB();
        } else if (strName == "search_string") {
            m_SearchString = strValue;
        } else if (strName == "all_graphs_signals_search_string") {
            m_AllGraphSignalsSearchString = strValue;
        } else if (strName == "log_values_to_hide") {
            m_LogValuesToHide = strValue;
        } else if (strName == "log_2nd_values_to_hide") {
            m_Log2ndValuesToHide = strValue;
        } else if (strName == "hide_some_log_values") {
            m_HideSomeLogValues = ez::ivariant(strValue).GetB();
        } else if (strName == "hide_some_log_2nd_values") {
            m_HideSomeLog2ndValues = ez::ivariant(strValue).GetB();
        } else if (strName == "signals_preview_count_x") {
            m_SignalPreview_CountX = ez::uvariant(strValue).GetU();
        } else if (strName == "signals_preview_size_x") {
            m_SignalPreview_SizeX = ez::fvariant(strValue).GetF();
        } else if (strName == "graph_diff_first_mark") {
            m_DiffFirstMark = ez::fvariant(strValue).GetD();
        } else if (strName == "graph_diff_second_mark") {
            m_DiffSecondMark = ez::fvariant(strValue).GetD();
        } else if (strName == "graph_synchronize") {
            m_SyncGraphs = ez::ivariant(strValue).GetB();
        } else if (strName == "graph_sync_limits") {
            // dvariant::GetV4() now returns ez::math::dvec4 — ImPlotRect needs explicit unpacking
            const auto v = ez::dvariant(strValue).GetV4();
            m_SyncGraphsLimits = ImPlotRect(v.x, v.y, v.z, v.w);
        } else if (strName == "code_file_path_name") {
            m_CodeFilePathName = strValue;
        } else if (strName == "curve_radius_detection") {
            m_CurveRadiusDetection = ez::dvariant(strValue).GetD();
        } else if (strName == "selected_curve_display_thickness") {
            m_SelectedCurveDisplayThickNess = ez::dvariant(strValue).GetD();
        } else if (strName == "default_curve_display_thickness") {
            m_DefaultCurveDisplayThickNess = ez::dvariant(strValue).GetD();
        } else if (strName == "use_predefined_zero_value") {
            m_UsePredefinedZeroValue = ez::ivariant(strValue).GetB();
        } else if (strName == "predefined_zero_value") {
            m_PredefinedZeroValue = ez::dvariant(strValue).GetD();
        } else if (strName == "show_variable_signals_in_all_graph_view") {
            m_ShowVariableSignalsInAllGraphView = ez::dvariant(strValue).GetB();
        } else if (strName == "show_variable_signals_in_graph_view") {
            m_ShowVariableSignalsInGraphView = ez::dvariant(strValue).GetB();
        } else if (strName == "show_variable_signals_in_hovered_list_view") {
            m_ShowVariableSignalsInHoveredListView = ez::dvariant(strValue).GetB();
        } else if (strName == "show_variable_signals_in_log_view") {
            m_ShowVariableSignalsInLogView = ez::dvariant(strValue).GetB();
        } else if (strName == "show_variable_signals_in_log_2nd_view") {
            m_ShowVariableSignalsInLog2ndView = ez::dvariant(strValue).GetB();
        } else if (strName == "auto_resize_columns_log_view") {
            m_AutoResizeLogColumns = ez::dvariant(strValue).GetB();
        } else if (strName == "auto_resize_columns_log_2nd_view") {
            m_AutoResizeLog2ndColumns = ez::dvariant(strValue).GetB();
        } else if (strName == "hovered_list_changed_text_rect_thickness") {
            m_HoveredListChangedTextRectThickNess = ez::fvariant(strValue).GetF();
        } else if (strName == "project_script_font_scale") {
            m_ProjectScriptFontScale = ez::fvariant(strValue).GetF();
            if (m_ProjectScriptFontScale <= 0.0f) m_ProjectScriptFontScale = 1.0f;
        } else if (strName == "last_log_file_path") {
            m_LastLogFilePath = strValue;
        } else if (strName == "script_file") {
            SetScriptFilePathName(ez::xml::Node::unEscapeXml(strValue));
        } 
    } else if (strParentName == "log_files") {
        if (strName == "log_file") {
            AddSourceFilePathName(ez::xml::Node::unEscapeXml(strValue));
        }
    }

    ImLayout::ref().setFromXmlNodes(vNode, vParent, "project");
    ScriptingEngine::ref()->setFromXmlNodes(vNode, vParent, "project");
    LogEngine::ref()->setFromXmlNodes(vNode, vParent, "project");
    SettingsDialog::ref().setFromXmlNodes(vNode, vParent, "project");

    return true;
}

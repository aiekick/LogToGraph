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

#include <models/lua/LuaEngine.h>
#include <models/log/LogEngine.h>
#include <panes/CodePane.h>
#include <models/database/DataBase.h>
#include <models/graphs/GraphView.h>
#include <models/graphs/GraphGroup.h>
#include <panes/LogPane.h>
#include <panes/ToolPane.h>
#include <panes/LogPaneSecondView.h>
#include <panes/GraphListPane.h>
#include <panes/GraphGroupPane.h>
#include <panes/SignalsHoveredDiff.h>
#include <panes/SignalsHoveredList.h>
#include <panes/SignalsHoveredMap.h>

#include <Panes/GraphPane.h>
#include <LayoutManager.h>
#include <Systems/SettingsDialog.h>

#include <EzLibs/EzFile.hpp>

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
    m_IsLoaded = false;
    m_IsThereAnyChanges = false;
    Messaging::Instance()->Clear();
}

void ProjectFile::ClearDatas() {
    LuaEngine::Instance()->Clear();
    LogEngine::Instance()->Clear();
    GraphView::Instance()->Clear();
    GraphGroup::Instance()->Clear();
    ToolPane::Instance()->Clear();
    LogPane::Instance()->Clear();
    LogPaneSecondView::Instance()->Clear();
    GraphListPane::Instance()->Clear();
    GraphGroupPane::Instance()->Clear();
    SignalsHoveredDiff::Instance()->Clear();
    SignalsHoveredList::Instance()->Clear();
    SignalsHoveredMap::Instance()->Clear();
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
    DataBase::Instance()->CreateDBFile(m_ProjectFilePathName);
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
        if (DataBase::Instance()->IsFileASqlite3DB(filePathName)) {
            if (DataBase::Instance()->OpenDBFile(filePathName)) {
                ClearDatas();
                auto xml_settings = DataBase::Instance()->GetSettingsXMLDatas();
                if (LoadConfigString(ez::xml::Node::unEscapeXml(xml_settings), "") || xml_settings.empty()) {
                    m_ProjectFilePathName = ez::file::simplifyFilePath(vFilePathName);
                    auto ps = ez::file::parsePathFileName(m_ProjectFilePathName);
                    if (ps.isOk) {
                        m_ProjectFileName = ps.name;
                        m_ProjectFilePath = ps.path;
                        CodePane::Instance()->OpenFile(m_CodeFilePathName);
                    }
                    m_IsLoaded = true;
                    SetProjectChange(false);
                } else {
                    Clear();
                    LogVarError("The project file %s cant be loaded", filePathName.c_str());
                }

                LogEngine::Instance()->Finalize();
                GraphListPane::Instance()->UpdateDB();
                ToolPane::Instance()->UpdateTree();
                LogEngine::Instance()->PrepareAfterLoad();

                DataBase::Instance()->CloseDBFile();
            }
        }
    }
    return m_IsLoaded;
}
bool ProjectFile::Save() {
    if (m_NeverSaved) {
        return false;
    }

    LogEngine::Instance()->PrepareForSave();
    if (DataBase::Instance()->OpenDBFile(m_ProjectFilePathName)) {
        auto xml_settings = ez::xml::Node::escapeXml(SaveConfigString("", "config"));
        if (DataBase::Instance()->SetSettingsXMLDatas(xml_settings)) {
            SetProjectChange(false);
            DataBase::Instance()->CloseDBFile();
            return true;
        }
        DataBase::Instance()->CloseDBFile();
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

ez::xml::Nodes ProjectFile::getXmlNodes(const std::string& /*vUserDatas*/) {
    ez::xml::Node node;

    node.setName("project");
    node.addChilds(LayoutManager::Instance()->getXmlNodes("project"));
    node.addChilds(LuaEngine::Instance()->getXmlNodes("project"));
    node.addChilds(LogEngine::Instance()->getXmlNodes("project"));
    node.addChild("graph_bar_colors").setContent(m_GraphColors.graphBarColor);
    node.addChild("graph_bar_colors").setContent(m_GraphColors.graphBarColor);
    node.addChild("graph_current_time_colors").setContent(m_GraphColors.graphHoveredTimeColor);
    node.addChild("graph_mouse_current_time_colors").setContent(m_GraphColors.graphMouseHoveredTimeColor);
    node.addChild("graph_diff_first_mark_color").setContent(m_GraphColors.graphFirstDiffMarkColor);
    node.addChild("graph_diff_second_mark_color").setContent(m_GraphColors.graphSecondDiffMarkColor);
    node.addChild("selection_collapsing").setContent(m_CollapseLogSelection);
    node.addChild("auto_colorize").setContent(m_AutoColorize);
    node.addChild("search_string").setContent(m_SearchString);
    node.addChild("all_graphs_signals_search_string").setContent(m_AllGraphSignalsSearchString);
    node.addChild("values_to_hide").setContent(m_ValuesToHide);
    node.addChild("hide_some_values").setContent(m_HideSomeValues);
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
    node.addChild("last_log_file_path").setContent(m_LastLogFilePath);

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
        } else if (strName == "selection_collapsing") {
            m_CollapseLogSelection = ez::ivariant(strValue).GetB();
        } else if (strName == "auto_colorize") {
            m_AutoColorize = ez::ivariant(strValue).GetB();
        } else if (strName == "search_string") {
            m_SearchString = strValue;
        } else if (strName == "all_graphs_signals_search_string") {
            m_AllGraphSignalsSearchString = strValue;
        } else if (strName == "values_to_hide") {
            m_ValuesToHide = strValue;
        } else if (strName == "hide_some_values") {
            m_HideSomeValues = ez::ivariant(strValue).GetB();
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
            auto v4 = ez::dvariant(strValue).GetV4();
            m_SyncGraphsLimits.X.Min = v4.x;
            m_SyncGraphsLimits.X.Max = v4.y;
            m_SyncGraphsLimits.Y.Min = v4.z;
            m_SyncGraphsLimits.Y.Max = v4.w;
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
        } else if (strName == "last_log_file_path") {
            m_LastLogFilePath = strValue;
        }
    }

    LayoutManager::Instance()->setFromXmlNodes(vNode, vParent, "project");
    LuaEngine::Instance()->setFromXmlNodes(vNode, vParent, "project");
    LogEngine::Instance()->setFromXmlNodes(vNode, vParent, "project");

    return true;
}

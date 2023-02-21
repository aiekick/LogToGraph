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

#include <ctools/FileHelper.h>
#include <ctools/cTools.h>
#include <Panes/Manager/LayoutManager.h>
#include <Engine/Lua/LuaEngine.h>
#include <Engine/Log/LogEngine.h>
#include <Panes/CodePane.h>
#include <Engine/DB/DBEngine.h>
#include <Engine/Graphs/GraphView.h>
#include <Panes/LogPane.h>
#include <Panes/ToolPane.h>
#include <Panes/LogPaneSecondView.h>
#include <Panes/GraphListPane.h>
#include <Panes/GraphGroupPane.h>
#include <Panes/SignalsHoveredDiff.h>
#include <Panes/SignalsHoveredList.h>
#include <Panes/SignalsHoveredMap.h>

/*ProjectFile::ProjectFile(const std::string & vFilePathName)
{
	PathName = FileHelper::Instance()->SimplifyFilePath(vFilePathName);
	auto ps = FileHelper::Instance()->ParsePathFileName(PathName);
	if (ps.isOk)
	{
		Path = ps.path;
	}
}*/

void ProjectFile::Clear()
{
	m_ProjectFilePathName.clear();
	m_ProjectFilePath.clear();
	m_IsLoaded = false;
	m_NeverSaved = true;
	m_IsThereAnyNotSavedChanged = false;
}

void ProjectFile::ClearDatas()
{
	LuaEngine::Instance()->Clear();
	LogEngine::Instance()->Clear();
	GraphView::Instance()->Clear();
	ToolPane::Instance()->Clear();
	LogPane::Instance()->Clear();
	LogPaneSecondView::Instance()->Clear();
	GraphListPane::Instance()->Clear();
	GraphGroupPane::Instance()->Clear();
	SignalsHoveredDiff::Instance()->Clear();
	SignalsHoveredList::Instance()->Clear();
	SignalsHoveredMap::Instance()->Clear();
}

void ProjectFile::New()
{
	Clear();
	ClearDatas();
	m_IsLoaded = true;
}

void ProjectFile::New(const std::string& vFilePathName)
{
	Clear();
	ClearDatas();
	m_ProjectFilePathName = FileHelper::Instance()->SimplifyFilePath(vFilePathName);
	DBEngine::Instance()->CreateDBFile(m_ProjectFilePathName);
	auto ps = FileHelper::Instance()->ParsePathFileName(m_ProjectFilePathName);
	if (ps.isOk)
	{
		m_ProjectFilePath = ps.path;
	}
	m_IsLoaded = true;
	if (!vFilePathName.empty())
	{
		m_NeverSaved = false;
		SetProjectChange(false);
	}
}

bool ProjectFile::Load()
{
	return LoadAs(m_ProjectFilePathName);
}

// ils wanted to not pass the adress for re open case
// else, the clear will set vFilePathName to empty because with re open, target PathName
bool ProjectFile::LoadAs(const std::string& vFilePathName)
{
	Clear();
	std::string filePathName = FileHelper::Instance()->SimplifyFilePath(vFilePathName);
	if (DBEngine::Instance()->IsFileASqlite3DB(filePathName))
	{
		if (DBEngine::Instance()->OpenDBFile(filePathName))
		{
			ClearDatas();

			auto xml_settings = DBEngine::Instance()->GetSettingsXMLDatas();
			if (LoadConfigString(unEscapeXmlCode(xml_settings)) == tinyxml2::XMLError::XML_SUCCESS)
			{
				m_ProjectFilePathName = filePathName;
				auto ps = FileHelper::Instance()->ParsePathFileName(m_ProjectFilePathName);
				if (ps.isOk)
				{
					m_ProjectFilePath = ps.path;
					CodePane::Instance()->SetCodeFile(m_CodeFilePathName);
					LogEngine::Instance()->Finalize();
					//LuaEngine::Instance()->StartWorkerThread(true);
				}
				m_IsLoaded = true;
				m_NeverSaved = false;
				SetProjectChange(false);
			}
			else
			{
				Clear();
			}

			LogEngine::Instance()->Finalize();
			GraphListPane::Instance()->UpdateDB();
			ToolPane::Instance()->UpdateTree();
			LogEngine::Instance()->PrepareAfterLoad();

			DBEngine::Instance()->CloseDBFile();
		}
	}
	else
	{
		Clear();
	}

	return m_IsLoaded;
}

bool ProjectFile::Save()
{
	if (m_NeverSaved) 
		return false;

	LogEngine::Instance()->PrepareForSave();

	if (DBEngine::Instance()->OpenDBFile(m_ProjectFilePathName))
	{
		auto xml_settings = escapeXmlCode(SaveConfigString());
		if (DBEngine::Instance()->SetSettingsXMLDatas(xml_settings))
		{
			SetProjectChange(false);

			DBEngine::Instance()->CloseDBFile();

			return true;
		}

		DBEngine::Instance()->CloseDBFile();
	}	

	return false;
}

bool ProjectFile::SaveAs(const std::string& vFilePathName)
{
	std::string filePathName = FileHelper::Instance()->SimplifyFilePath(vFilePathName);
	auto ps = FileHelper::Instance()->ParsePathFileName(filePathName);
	if (ps.isOk)
	{
		m_ProjectFilePathName = FileHelper::Instance()->ComposePath(ps.path, ps.name, APP_PROJECT_FILE_EXT);
		m_ProjectFilePath = ps.path;
		m_NeverSaved = false;
		return Save();
	}
	return false;
}

bool ProjectFile::IsLoaded() const
{
	return m_IsLoaded;
}

bool ProjectFile::IsNeverSaved() const
{
	return m_NeverSaved;
}

bool ProjectFile::IsThereAnyNotSavedChanged() const
{
	return m_IsThereAnyNotSavedChanged;
}

void ProjectFile::SetProjectChange(const bool& vChange)
{
	m_IsThereAnyNotSavedChanged = vChange;
}

std::string ProjectFile::getXml(const std::string& vOffset, const std::string& /*vUserDatas*/)
{
	std::string str;

	str += vOffset + "<project>\n";
	str += LayoutManager::Instance()->getXml(vOffset + "\t", "project");
	str += LuaEngine::Instance()->getXml(vOffset + "\t", "project");
	str += LogEngine::Instance()->getSignalVisibilty(vOffset + "\t", "project");
	str += vOffset + "\t<graph_bar_colors>" + ct::toStrFromImVec4(m_GraphColors.graphBarColor) + "</graph_bar_colors>\n";
	str += vOffset + "\t<graph_current_time_colors>" + ct::toStrFromImVec4(m_GraphColors.graphHoveredTimeColor) + "</graph_current_time_colors>\n";
	str += vOffset + "\t<graph_mouse_current_time_colors>" + ct::toStrFromImVec4(m_GraphColors.graphMouseHoveredTimeColor) + "</graph_mouse_current_time_colors>\n";
	str += vOffset + "\t<graph_diff_first_mark_color>" + ct::toStrFromImVec4(m_GraphColors.graphFirstDiffMarkColor) + "</graph_diff_first_mark_color>\n";
	str += vOffset + "\t<graph_diff_second_mark_color>" + ct::toStrFromImVec4(m_GraphColors.graphSecondDiffMarkColor) + "</graph_diff_second_mark_color>\n";
	str += vOffset + "\t<selection_collapsing>" + (m_CollapseLogSelection ? "true" : "false") + "</selection_collapsing>\n";
	str += vOffset + "\t<auto_colorize>" + (m_AutoColorize ? "true" : "false") + "</auto_colorize>\n";
	str += vOffset + "\t<search_string>" + m_SearchString + "</search_string>\n";
	str += vOffset + "\t<all_graphs_signals_search_string>" + m_AllGraphSignalsSearchString + "</all_graphs_signals_search_string>\n";
	str += vOffset + "\t<values_to_hide>" + m_ValuesToHide + "</values_to_hide>\n";
	str += vOffset + "\t<hide_some_values>" + (m_HideSomeValues  ? "true" : "false") + "</hide_some_values>\n";
	str += vOffset + "\t<signals_preview_count_x>" + ct::toStr(m_SignalPreview_CountX) + "</signals_preview_count_x>\n";
	str += vOffset + "\t<signals_preview_size_x>" + ct::toStr(m_SignalPreview_SizeX) + "</signals_preview_size_x>\n";
	str += vOffset + "\t<graph_diff_first_mark>" + ct::toStr("%f", m_DiffFirstMark) + "</graph_diff_first_mark>\n";
	str += vOffset + "\t<graph_diff_second_mark>" + ct::toStr("%f", m_DiffSecondMark) + "</graph_diff_second_mark>\n";
	str += vOffset + "\t<graph_synchronize>" + (m_SyncGraphs ? "true" : "false") + "</graph_synchronize>\n";
	str += vOffset + "\t<graph_sync_limits>" + ct::toStr("%f;%f;%f;%f", m_SyncGraphsLimits.X.Min, m_SyncGraphsLimits.X.Max, m_SyncGraphsLimits.Y.Min, m_SyncGraphsLimits.Y.Max) + "</graph_sync_limits>\n";
	str += vOffset + "\t<code_file_path_name>" + m_CodeFilePathName + "</code_file_path_name>\n";
	str += vOffset + "\t<curve_radius_detection>" + ct::toStr(m_CurveRadiusDetection) + "</curve_radius_detection>\n";
	str += vOffset + "\t<selected_curve_display_thickness>" + ct::toStr(m_SelectedCurveDisplayThickNess) + "</selected_curve_display_thickness>\n";
	str += vOffset + "\t<default_curve_display_thickness>" + ct::toStr(m_DefaultCurveDisplayThickNess) + "</default_curve_display_thickness>\n";
	str += vOffset + "\t<use_predefined_zero_value>" + (m_UsePredefinedZeroValue ? "true" : "false") + "</use_predefined_zero_value>\n";
	str += vOffset + "\t<predefined_zero_value>" + ct::toStr(m_PredefinedZeroValue) + "</predefined_zero_value>\n";
	str += vOffset + "\t<last_log_file_path>" + m_LastLogFilePath + "</last_log_file_path>\n";
	str += vOffset + "</project>\n";

	return str;
}

bool ProjectFile::setFromXml(tinyxml2::XMLElement* vElem, tinyxml2::XMLElement* vParent, const std::string& /*vUserDatas*/)
{
	// The value of this child identifies the name of this element
	std::string strName;
	std::string strValue;
	std::string strParentName;

	strName = vElem->Value();
	if (vElem->GetText())
		strValue = vElem->GetText();
	if (vParent != nullptr)
		strParentName = vParent->Value();

	if (strParentName == "project")
	{
		if (strName == "graph_bar_colors")
		{
            m_GraphColors.graphBarColor = ct::toImVec4(ct::fvariant(strValue).GetV4());
		}
		else if (strName == "graph_current_time_colors")
		{
            m_GraphColors.graphHoveredTimeColor = ct::toImVec4(ct::fvariant(strValue).GetV4());
		}
		else if (strName == "graph_mouse_current_time_colors")
		{
            m_GraphColors.graphMouseHoveredTimeColor = ct::toImVec4(ct::fvariant(strValue).GetV4());
		}
		else if (strName == "graph_diff_first_mark_color")
		{
            m_GraphColors.graphFirstDiffMarkColor = ct::toImVec4(ct::fvariant(strValue).GetV4());
		}
		else if (strName == "graph_diff_second_mark_color")
		{
            m_GraphColors.graphSecondDiffMarkColor = ct::toImVec4(ct::fvariant(strValue).GetV4());
		}
		else if (strName == "selection_collapsing")
		{
			m_CollapseLogSelection = ct::ivariant(strValue).GetB();
		}
		else if (strName == "auto_colorize")
		{
			m_AutoColorize = ct::ivariant(strValue).GetB();
		}
		else if (strName == "search_string")
		{
			m_SearchString = strValue;
		}
		else if (strName == "all_graphs_signals_search_string")
		{
			m_AllGraphSignalsSearchString = strValue;
		}
		else if (strName == "values_to_hide")
		{
			m_ValuesToHide = strValue;
		}
		else if (strName == "hide_some_values")
		{
			m_HideSomeValues = ct::ivariant(strValue).GetB();
		}
		else if (strName == "signals_preview_count_x")
		{
			m_SignalPreview_CountX = ct::uvariant(strValue).GetU();
		}
		else if (strName == "signals_preview_size_x")
		{
			m_SignalPreview_SizeX = ct::fvariant(strValue).GetF();
		}
		else if (strName == "graph_diff_first_mark")
		{
			m_DiffFirstMark = ct::fvariant(strValue).GetD();
		}
		else if (strName == "graph_diff_second_mark")
		{
			m_DiffSecondMark = ct::fvariant(strValue).GetD();
		}
		else if (strName == "graph_synchronize")
		{
			m_SyncGraphs = ct::ivariant(strValue).GetB();
		}
		else if (strName == "graph_sync_limits")
		{
			auto v4 = ct::dvariant(strValue).GetV4();
			m_SyncGraphsLimits.X.Min = v4.x;
			m_SyncGraphsLimits.X.Max = v4.y;
			m_SyncGraphsLimits.Y.Min = v4.z;
			m_SyncGraphsLimits.Y.Max = v4.w;
		}
		else if (strName == "code_file_path_name")
		{
			m_CodeFilePathName = strValue;
		}
		else if (strName == "curve_radius_detection")
		{
			m_CurveRadiusDetection = ct::dvariant(strValue).GetD();
		}
		else if (strName == "selected_curve_display_thickness")
		{
			m_SelectedCurveDisplayThickNess = ct::dvariant(strValue).GetD();
		}
		else if (strName == "default_curve_display_thickness")
		{
			m_DefaultCurveDisplayThickNess = ct::dvariant(strValue).GetD();
		}
		else if (strName == "use_predefined_zero_value")
		{
			m_UsePredefinedZeroValue = ct::ivariant(strValue).GetB();
		}
		else if (strName == "predefined_zero_value")
		{
			m_PredefinedZeroValue = ct::dvariant(strValue).GetD();
		}
		else if (strName == "last_log_file_path")
		{
			m_LastLogFilePath = strValue;
		}
	}
	
	LayoutManager::Instance()->setFromXml(vElem, vParent, "project");
	LuaEngine::Instance()->setFromXml(vElem, vParent, "project");
	LogEngine::Instance()->setSignalVisibilty(vElem, vParent, "project");

	return true;
}

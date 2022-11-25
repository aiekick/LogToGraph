// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/*
 * Copyright 2020 Stephane Cuillerdier (aka Aiekick)
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

/*ProjectFile::ProjectFile(const std::string & vFilePathName)
{
	ProjectFile::Instance()->PathName = FileHelper::Instance()->SimplifyFilePath(vFilePathName);
	auto ps = FileHelper::Instance()->ParsePathFileName(ProjectFile::Instance()->PathName);
	if (ps.isOk)
	{
		ProjectFile::Instance()->Path = ps.path;
	}
}*/

void ProjectFile::Clear()
{
	ProjectFile::Instance()->m_ProjectFilePathName.clear();
	ProjectFile::Instance()->m_ProjectFilePath.clear();
	m_IsLoaded = false;
	m_NeverSaved = true;
	m_IsThereAnyNotSavedChanged = false;
}

void ProjectFile::New()
{
	Clear();
	m_IsLoaded = true;
}

void ProjectFile::New(const std::string& vFilePathName)
{
	Clear();
	ProjectFile::Instance()->m_ProjectFilePathName = FileHelper::Instance()->SimplifyFilePath(vFilePathName);
	auto ps = FileHelper::Instance()->ParsePathFileName(ProjectFile::Instance()->m_ProjectFilePathName);
	if (ps.isOk)
	{
		ProjectFile::Instance()->m_ProjectFilePath = ps.path;
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
	return LoadAs(ProjectFile::Instance()->m_ProjectFilePathName);
}

// ils wanted to not pass the adress for re open case
// else, the clear will set vFilePathName to empty because with re open, target ProjectFile::Instance()->PathName
bool ProjectFile::LoadAs(const std::string vFilePathName)  
{
	Clear();
	std::string filePathName = FileHelper::Instance()->SimplifyFilePath(vFilePathName);
	if (LoadConfigFile(filePathName) == tinyxml2::XMLError::XML_SUCCESS)
	{
		ProjectFile::Instance()->m_ProjectFilePathName = filePathName;
		auto ps = FileHelper::Instance()->ParsePathFileName(ProjectFile::Instance()->m_ProjectFilePathName);
		if (ps.isOk)
		{
			ProjectFile::Instance()->m_ProjectFilePath = ps.path;
			LuaEngine::Instance()->ExecScriptOnFile();
			LogEngine::Instance()->PrepareAfterLoad();
		}
		m_IsLoaded = true;
		m_NeverSaved = false;
		SetProjectChange(false);
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

	if (SaveConfigFile(ProjectFile::Instance()->m_ProjectFilePathName))
	{
		SetProjectChange(false);
		return true;
	}

	return false;
}

bool ProjectFile::SaveAs(const std::string& vFilePathName)
{
	std::string filePathName = FileHelper::Instance()->SimplifyFilePath(vFilePathName);
	auto ps = FileHelper::Instance()->ParsePathFileName(filePathName);
	if (ps.isOk)
	{
		ProjectFile::Instance()->m_ProjectFilePathName = FileHelper::Instance()->ComposePath(ps.path, ps.name, "lt");
		ProjectFile::Instance()->m_ProjectFilePath = ps.path;
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

void ProjectFile::SetProjectChange(bool vChange)
{
	m_IsThereAnyNotSavedChanged = vChange;
}

std::string ProjectFile::GetAbsolutePath(const std::string& vFilePathName) const
{
	std::string res = vFilePathName;

	if (!vFilePathName.empty())
	{
		if (!FileHelper::Instance()->IsAbsolutePath(vFilePathName)) // relative
		{
			res = FileHelper::Instance()->SimplifyFilePath(
				ProjectFile::Instance()->m_ProjectFilePath + FileHelper::Instance()->puSlashType + vFilePathName);
		}
	}

	return res;
}

std::string ProjectFile::GetRelativePath(const std::string& vFilePathName) const
{
	std::string res = vFilePathName;

	if (!vFilePathName.empty())
	{
		res = FileHelper::Instance()->GetRelativePathToPath(vFilePathName, ProjectFile::Instance()->m_ProjectFilePath);
	}

	return res;
}

std::string ProjectFile::getXml(const std::string& vOffset, const std::string& /*vUserDatas*/)
{
	std::string str;

	str += vOffset + "<project>\n";
	str += LayoutManager::Instance()->getXml(vOffset + "\t", "project");
	str += LuaEngine::Instance()->getXml(vOffset + "\t", "project");
	str += LogEngine::Instance()->getSignalVisibilty(vOffset + "\t", "project");
	str += vOffset + "\t<graph_bar_colors>" + ct::toStrFromImVec4(m_GraphBarColor) + "</graph_bar_colors>\n";
	str += vOffset + "\t<graph_current_time_colors>" + ct::toStrFromImVec4(m_GraphHoveredTimeColor) + "</graph_current_time_colors>\n";
	str += vOffset + "\t<selection_collapsing>" + (m_CollapseLogSelection ? "true" : "false") + "</selection_collapsing>\n";
	str += vOffset + "\t<search_string>" + m_SearchString + "</search_string>\n";
	str += vOffset + "\t<values_to_hide>" + m_ValuesToHide + "</values_to_hide>\n";
	str += vOffset + "\t<hide_some_values>" + (m_HideSomeValues  ? "true" : "false") + "</hide_some_values>\n";
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
			m_GraphBarColor = ct::toImVec4(ct::fvariant(strValue).GetV4());
		else if (strName == "graph_current_time_colors")
			m_GraphHoveredTimeColor = ct::toImVec4(ct::fvariant(strValue).GetV4());
		else if (strName == "selection_collapsing")
			m_CollapseLogSelection = ct::ivariant(strValue).GetB();
		else if (strName == "search_string")
			m_SearchString = strValue;
		else if (strName == "values_to_hide")
			m_ValuesToHide = strValue;
		else if (strName == "hide_some_values")
			m_HideSomeValues = ct::ivariant(strValue).GetB();
	}
	
	LayoutManager::Instance()->setFromXml(vElem, vParent, "project");
	LuaEngine::Instance()->setFromXml(vElem, vParent, "project");
	LogEngine::Instance()->setSignalVisibilty(vElem, vParent, "project");

	return true;
}

ImVec4 ProjectFile::GetColorFromInteger(uint32_t /*vInteger*/) const
{
	ImVec4 res;

	if (IsLoaded())
	{

	}

	return res;
}

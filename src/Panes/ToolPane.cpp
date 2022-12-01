/*
Copyright 2022-2022 Stephane Cuillerdier (aka aiekick)

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
#include <Gui/MainFrame.h>
#include <ctools/cTools.h>
#include <ctools/FileHelper.h>
#include <Contrib/ImWidgets/ImWidgets.h>
#include <Project/ProjectFile.h>
#include <Project/ProjectFile.h>
#include <imgui/imgui_internal.h>
#include <Panes/Manager/LayoutManager.h>
#include <ImGuiFileDialog/ImGuiFileDialog.h>
#include <Engine/Lua/LuaEngine.h>
#include <Engine/Log/LogEngine.h>
#include <cinttypes> // printf zu
#include <Panes/LogPane.h>
#include <Panes/CodePane.h>

static int SourcePane_WidgetId = 0;

///////////////////////////////////////////////////////////////////////////////////
//// IMGUI PANE ///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////

bool ToolPane::Init()
{
	return true;
}

void ToolPane::Unit()
{

}

int ToolPane::DrawPanes(const uint32_t& /*vCurrentFrame*/, int vWidgetId, std::string /*vUserDatas*/, PaneFlags& vInOutPaneShown)
{
	SourcePane_WidgetId = vWidgetId;

	if (vInOutPaneShown & m_PaneFlag)
	{
		static ImGuiWindowFlags flags =
			ImGuiWindowFlags_NoCollapse |
			ImGuiWindowFlags_NoBringToFrontOnFocus;
		if (ImGui::Begin<PaneFlags>(m_PaneName,
			&vInOutPaneShown , m_PaneFlag, flags))
		{
#ifdef USE_DECORATIONS_FOR_RESIZE_CHILD_WINDOWS
			auto win = ImGui::GetCurrentWindowRead();
			if (win->Viewport->Idx != 0)
				flags |= ImGuiWindowFlags_NoResize;
			else
				flags =	ImGuiWindowFlags_NoCollapse |
				ImGuiWindowFlags_NoBringToFrontOnFocus;
#endif
			if (ProjectFile::Instance()->IsLoaded())
			{
				DrawTable();

				ImGui::Separator();

				DrawTree();
			}
		}

		ImGui::End();
	}

	return SourcePane_WidgetId;
}

void ToolPane::DrawDialogsAndPopups(const uint32_t& /*vCurrentFrame*/, std::string /*vUserDatas*/)
{
	if (ProjectFile::Instance()->IsLoaded())
	{
		ImVec2 maxSize = MainFrame::Instance()->m_DisplaySize;
		ImVec2 minSize = maxSize * 0.5f;

		if (ImGuiFileDialog::Instance()->Display("OPEN_LUA_SCRIPT_FILE",
			ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking,
			minSize, maxSize))
		{
			if (ImGuiFileDialog::Instance()->IsOk())
			{
				LuaEngine::Instance()->SetLuaFilePathName(ImGuiFileDialog::Instance()->GetFilePathName());
				CodePane::Instance()->SetCodeFile(ImGuiFileDialog::Instance()->GetFilePathName());
				ProjectFile::Instance()->SetProjectChange();
			}

			ImGuiFileDialog::Instance()->Close();
		}
		
		if (ImGuiFileDialog::Instance()->Display("OPEN_LOG_FILE",
			ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking,
			minSize, maxSize))
		{
			if (ImGuiFileDialog::Instance()->IsOk())
			{
				LuaEngine::Instance()->SetLogFilePathName(ImGuiFileDialog::Instance()->GetFilePathName());
				ProjectFile::Instance()->SetProjectChange();
			}

			ImGuiFileDialog::Instance()->Close();
		}
	}
}

int ToolPane::DrawWidgets(const uint32_t& /*vCurrentFrame*/, int vWidgetId, std::string /*vUserDatas*/)
{
	return vWidgetId;
}

void ToolPane::UpdateTree()
{
	PrepareLogAfterSearch(ProjectFile::Instance()->m_SearchString);
}

void ToolPane::DrawTable()
{
	ImGui::Header("Lua Script File");
	if (ImGui::ContrastedButton("Choose a Lua Script File", nullptr, nullptr, -1.0f, ImVec2(-1.0f, 0.0f)))
	{
		ImGuiFileDialog::Instance()->OpenDialog("OPEN_LUA_SCRIPT_FILE", "Open a Lua Script File", ".lua,.*",
			LuaEngine::Instance()->GetLuaFilePathName(), 1, nullptr, ImGuiFileDialogFlags_Modal);
	}
	ImGui::TextWrapped(LuaEngine::Instance()->GetLuaFilePathName().c_str());

	ImGui::Separator();

	ImGui::Header("Log File");
	if (ImGui::ContrastedButton("Choose a Log File", nullptr, nullptr, -1.0f, ImVec2(-1.0f, 0.0f)))
	{
		ImGuiFileDialog::Instance()->OpenDialog("OPEN_LOG_FILE", "Open a Log File", ".*",
			LuaEngine::Instance()->GetLuaFilePathName(), 1, nullptr, ImGuiFileDialogFlags_Modal);
	}
	ImGui::TextWrapped(LuaEngine::Instance()->GetLogFilePathName().c_str());

	ImGui::Separator();

	ImGui::Header("Analyse");
	if (ImGui::ContrastedButton("Analyse log file", nullptr, nullptr, -1.0f, ImVec2(-1.0f, 0.0f)))
	{
		LogEngine::Instance()->PrepareForSave();
		LuaEngine::Instance()->ExecScriptOnFile();
		LogEngine::Instance()->PrepareAfterLoad();
	}
}

void ToolPane::DisplayItem(SignalDatas& vDatas)
{
	auto name_str = ct::toStr("%s (%u)", vDatas.name.c_str(), (uint32_t)vDatas.count_values);
	if (ImGui::Selectable(name_str.c_str(), vDatas.show))
	{
		vDatas.show = !vDatas.show;

		LogEngine::Instance()->ShowHideSignal(
			vDatas.category, vDatas.name, vDatas.show);

		if (ProjectFile::Instance()->m_CollapseLogSelection)
		{
			LogPane::Instance()->PrepareLog();
		}

		ProjectFile::Instance()->SetProjectChange();
	}
}

void ToolPane::DrawTree()
{
	auto& search_string = ProjectFile::Instance()->m_SearchString;

	ImGui::Header("Signals");

	bool _collapse_all = false;
	bool _expand_all = false;

	const float fw = ImGui::GetContentRegionAvail().x;
	
	if (search_string.empty())
	{
		const float aw = (fw - ImGui::GetStyle().ItemSpacing.x) * 0.5f;
		if (ImGui::ContrastedButton("Collapse All##ToolPaneDrawTree", nullptr, nullptr, aw))
		{
			_collapse_all = true;
		}

		ImGui::SameLine();

		if (ImGui::ContrastedButton("Expand All##ToolPaneDrawTree", nullptr, nullptr, aw))
		{
			_expand_all = true;
		}
	}

	if (ImGui::ContrastedButton("Hide All Graphs##ToolPaneDrawTree", nullptr, nullptr, fw))
	{
		HideAllGraphs();
	}

	ImGui::Text("Search : ");
	ImGui::SameLine();

	static char _search_buffer[1024 + 1] = "";
	snprintf(_search_buffer, 1024, "%s", search_string.c_str());
	if (ImGui::ContrastedButton("R##SearchDrawTree"))
	{
		search_string.clear();
		_search_buffer[0] = '\0';
	}
	ImGui::SameLine();
	if (ImGui::InputText("##Search", _search_buffer, 1024))
	{
		search_string = ct::toLower(_search_buffer);
		PrepareLogAfterSearch(search_string);
	}

	if (!search_string.empty())
	{
		// if first frame is not built
		if (m_CategoryLessDatas.empty())
		{
			PrepareLogAfterSearch(search_string);
		}

		ImGui::Indent();

		// affichage ordonné sans les categorie
		for (auto& item_name : m_CategoryLessDatas)
		{
			DisplayItem(item_name.second);
		}

		ImGui::Unindent();
	}
	else
	{
		// affichage arborescent ordonné par categorie
		for (auto& item_cat : *LogEngine::Instance())
		{
			if (_collapse_all)
			{
				ImGui::SetNextItemOpen(false);
			}

			if (_expand_all)
			{
				ImGui::SetNextItemOpen(true);
			}

			auto cat_str = ct::toStr("%s (%u)", item_cat.first.c_str(), (uint32_t)item_cat.second.size());
			if (ImGui::TreeNode(cat_str.c_str()))
			{
				ImGui::Indent();

				for (auto& item_name : item_cat.second)
				{
					DisplayItem(item_name.second);
				}

				ImGui::Unindent();

				ImGui::TreePop();
			}
		}
	}
}

void ToolPane::PrepareLogAfterSearch(const std::string& vSearchString)
{
	if (!vSearchString.empty())
	{
		m_CategoryLessDatas.clear();

		for (auto& item_cat : *LogEngine::Instance())
		{
			for (auto& item_name : item_cat.second)
			{
				if (item_name.second.low_case_name_for_search.find(vSearchString) == std::string::npos)
				{
					continue;
				}

				m_CategoryLessDatas[item_name.first] = item_name.second;
			}
		}
	}
}

void ToolPane::HideAllGraphs()
{
	bool _one_at_least = false;

	for (auto& item_cat : *LogEngine::Instance())
	{
		for (auto& item_name : item_cat.second)
		{
			if (item_name.second.show)
			{
				_one_at_least = true;
			}

			LogEngine::Instance()->ShowHideSignal(
				item_name.second.category, 
				item_name.second.name, 
				false);
		}
	}

	if (_one_at_least)
	{
		ProjectFile::Instance()->SetProjectChange();
	}
}

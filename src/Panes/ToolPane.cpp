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
#include <Gui/MainFrame.h>
#include <ctools/cTools.h>
#include <ctools/FileHelper.h>
#include <Contrib/ImWidgets/ImWidgets.h>
#include <Project/ProjectFile.h>
#include <imgui/imgui_internal.h>
#include <Panes/Manager/LayoutManager.h>
#include <ImGuiFileDialog/ImGuiFileDialog.h>
#include <cinttypes> // printf zu
#include <Panes/LogPane.h>
#include <Panes/CodePane.h>

#include <Engine/Lua/LuaEngine.h>
#include <Engine/Log/LogEngine.h>
#include <Engine/Log/SourceFile.h>
#include <Engine/Log/SignalSerie.h>
#include <Engine/Log/SignalTick.h>
#include <Engine/Graphs/GraphView.h>

static int SourcePane_WidgetId = 0;

///////////////////////////////////////////////////////////////////////////////////
//// IMGUI PANE ///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////

void ToolPane::Clear()
{
	m_SignalSeries.clear();
}

bool ToolPane::Init()
{
	return true;
}

void ToolPane::Unit()
{

}

int ToolPane::DrawPanes(const uint32_t& /*vCurrentFrame*/, const int& vWidgetId, const std::string& /*vvUserDatas*/, PaneFlag& vInOutPaneShown)
{
	SourcePane_WidgetId = vWidgetId;

	if (vInOutPaneShown & m_PaneFlag)
	{
		static ImGuiWindowFlags flags =
			ImGuiWindowFlags_NoCollapse |
			ImGuiWindowFlags_NoBringToFrontOnFocus;
		if (ImGui::Begin<PaneFlag>(m_PaneName,
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

void ToolPane::DrawDialogsAndPopups(const uint32_t& /*vCurrentFrame*/, const std::string& /*vvUserDatas*/)
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
				LuaEngine::Instance()->AddSourceFilePathName(ImGuiFileDialog::Instance()->GetFilePathName());
				ProjectFile::Instance()->SetProjectChange();
			}

			ImGuiFileDialog::Instance()->Close();
		}
	}
}

int ToolPane::DrawWidgets(const uint32_t& /*vCurrentFrame*/, const int& vWidgetId, const std::string& /*vvUserDatas*/)
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
	ImGui::TextWrapped("%s", LuaEngine::Instance()->GetLuaFilePathName().c_str());

	ImGui::Header("Log Files");

	if (ImGui::ContrastedButton("Open a Log File", nullptr, nullptr, -1.0f, ImVec2(-1.0f, 0.0f)))
	{
		ImGuiFileDialog::Instance()->OpenDialog("OPEN_LOG_FILE", "Open a Log File", ".*",
			LuaEngine::Instance()->GetLuaFilePathName(), 1, nullptr, ImGuiFileDialogFlags_Modal);
	}
	
	auto& container_ref = LuaEngine::Instance()->GetSourceFilePathNamesRef();

	if (!container_ref.empty())
	{
		static ImGuiTableFlags flags = ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders;
		if (ImGui::BeginTable("##sourcefilestable", 2, flags, ImVec2(-1.0f, container_ref.size() * ImGui::GetTextLineHeightWithSpacing())))
		{
			ImGui::TableSetupScrollFreeze(0, 1); // Make header always visible
			ImGui::TableSetupColumn("Log Files", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableSetupColumn("##Close", ImGuiTableColumnFlags_WidthFixed);
			ImGui::TableHeadersRow();

			auto it_to_erase = container_ref.end();
			for (auto it_source_file = container_ref.begin(); it_source_file != container_ref.end(); ++it_source_file)
			{
				ImGui::TableNextRow();

				if (ImGui::TableSetColumnIndex(0)) // first column
				{
					ImGui::Selectable(it_source_file->first.c_str());
				}

				if (ImGui::TableSetColumnIndex(1)) // second column
				{
					if (ImGui::ContrastedButton(ICON_NDP_CANCEL, nullptr, nullptr, 0.0f, ImVec2(0.0f, 0.0f)))
					{
						it_to_erase = it_source_file;
					}
				}
			}

			// erase
			if (it_to_erase != container_ref.end())
			{
				container_ref.erase(it_to_erase);
			}

			ImGui::EndTable();
		}
	}	

	ImGui::Header("Predefined Zero value");
	ImGui::CheckBoxBoolDefault("Use Predefined Zero Value ?", &ProjectFile::Instance()->m_UsePredefinedZeroValue, false);
	if (ProjectFile::Instance()->m_UsePredefinedZeroValue) 
	{
		ImGui::InputDouble("##Predefinedzerovalue", &ProjectFile::Instance()->m_PredefinedZeroValue);
	}

	ImGui::Header("Analyse");
	if (!LuaEngine::Instance()->IsJoinable())
	{
		if (ImGui::ContrastedButton("Start LuAnalyse of log file", nullptr, nullptr, -1.0f, ImVec2(-1.0f, 0.0f)))
		{
			LuaEngine::Instance()->StartWorkerThread(false);
		}
	}
	else
	{
		if (ImGui::ContrastedButton("Stop LuaAnalyse", nullptr, nullptr, -1.0f, ImVec2(-1.0f, 0.0f)))
		{
			LuaEngine::Instance()->StopWorkerThread();
		}

		auto progress = (float)LuaEngine::s_Progress;
		ImGui::ProgressBar(progress);
	}
}

void ToolPane::DisplayItem(const SignalSerieWeak& vDatasSerie)
{
	if (!vDatasSerie.expired())
	{
		auto ptr = vDatasSerie.lock();
		if (ptr)
		{
			auto name_str = ct::toStr("%s (%u)", ptr->name.c_str(), (uint32_t)ptr->count_base_records);
			if (ImGui::Selectable(name_str.c_str(), ptr->show))
			{
				ptr->show = !ptr->show;

				LogEngine::Instance()->ShowHideSignal(
					ptr->category, ptr->name, ptr->show);

				if (ProjectFile::Instance()->m_CollapseLogSelection)
				{
					LogPane::Instance()->PrepareLog();
				}

				ProjectFile::Instance()->SetProjectChange();
			}
		}
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
		if (ImGui::ContrastedButton("Collapse All##ToolPane_DrawTree", nullptr, nullptr, aw))
		{
			_collapse_all = true;
		}

		ImGui::SameLine();

		if (ImGui::ContrastedButton("Expand All##ToolPane_DrawTree", nullptr, nullptr, aw))
		{
			_expand_all = true;
		}
	}

	if (ImGui::ContrastedButton("Hide All Graphs##ToolPane_DrawTree", nullptr, nullptr, fw))
	{
		HideAllGraphs();
	}

	ImGui::Text("Search : ");
	ImGui::SameLine();

	snprintf(m_search_buffer, 1024, "%s", search_string.c_str());
	if (ImGui::ContrastedButton("R##ToolPane_DrawTree"))
	{
		search_string.clear();
		m_search_buffer[0] = '\0';
	}
	ImGui::SameLine();
	if (ImGui::InputText("##ToolPane_DrawTree_Search", m_search_buffer, 1024))
	{
		search_string = ct::toLower(m_search_buffer);
		PrepareLogAfterSearch(search_string);
	}

	if (ImGui::BeginChild("##Items_ToolPane_DrawTree"))
	{
		if (!search_string.empty())
		{
			// if first frame is not built
			if (m_SignalSeries.empty())
			{
				PrepareLogAfterSearch(search_string);
			}

			ImGui::Indent();

			// affichage ordonne sans les categorie
			for (auto& item_name : m_SignalSeries)
			{
				DisplayItem(item_name.second);
			}

			ImGui::Unindent();
		}
		else
		{
			// affichage arborescent ordonne par categorie
			for (auto& item_cat : LogEngine::Instance()->GetSignalSeries())
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
	ImGui::EndChild();
}

void ToolPane::PrepareLogAfterSearch(const std::string& vSearchString)
{
	if (!vSearchString.empty())
	{
		m_SignalSeries.clear();

		for (auto& item_cat : LogEngine::Instance()->GetSignalSeries())
		{
			for (auto& item_name : item_cat.second)
			{
				if (item_name.second)
				{
					if (item_name.second->low_case_name_for_search.find(vSearchString) == std::string::npos)
					{
						continue;
					}

					m_SignalSeries[item_name.first] = item_name.second;
				}
			}
		}
	}
}

void ToolPane::HideAllGraphs()
{
	bool _one_at_least = false;

	for (auto& item_cat : LogEngine::Instance()->GetSignalSeries())
	{
		for (auto& item_name : item_cat.second)
		{
			if (item_name.second)
			{
				if (item_name.second->show)
				{
					_one_at_least = true;
				}

				LogEngine::Instance()->ShowHideSignal(
					item_name.second->category,
					item_name.second->name,
					false);
			}
		}
	}

	if (_one_at_least)
	{
		GraphView::Instance()->Clear();
		ProjectFile::Instance()->SetProjectChange();
	}
}

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
#include <Project/ProjectFile.h>
#include <cinttypes> // printf zu
#include <panes/LogPane.h>
#include <panes/CodePane.h>

#include <models/lua/LuaEngine.h>
#include <models/log/LogEngine.h>
#include <models/log/SourceFile.h>
#include <models/log/SignalSerie.h>
#include <models/log/SignalTick.h>
#include <models/graphs/GraphView.h>

#include <ezlibs/ezFile.hpp>

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
				flags =	ImGuiWindowFlags_NoCollapse |
				ImGuiWindowFlags_NoBringToFrontOnFocus;
#endif
			if (ProjectFile::Instance()->IsLoaded())
			{
				DrawTable();

				DrawTree();
			}
		}

		ImGui::End();
	}

	return SourcePane_WidgetId;
}

bool ToolPane::DrawDialogsAndPopups(const uint32_t& /*vCurrentFrame*/, const ImRect& vRect, ImGuiContext* /*vContextPtr*/, void* /*vUserDatas*/) {
	if (ProjectFile::Instance()->IsLoaded())
	{
        ImVec2 maxSize = vRect.GetSize();
		ImVec2 minSize = maxSize * 0.5f;

		if (ImGuiFileDialog::Instance()->Display("OPEN_LUA_SCRIPT_FILE",
			ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking,
			minSize, maxSize))
		{
			if (ImGuiFileDialog::Instance()->IsOk())
			{
				LuaEngine::Instance()->SetLuaFilePathName(ImGuiFileDialog::Instance()->GetFilePathName());
				CodePane::Instance()->OpenFile(ImGuiFileDialog::Instance()->GetFilePathName());
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
				ProjectFile::Instance()->m_LastLogFilePath = ImGuiFileDialog::Instance()->GetFilePathName();
				auto files = ImGuiFileDialog::Instance()->GetSelection();
				for (const auto& item : files)
				{
					LuaEngine::Instance()->AddSourceFilePathName(item.second);
				}
				
				ProjectFile::Instance()->SetProjectChange();
			}

			ImGuiFileDialog::Instance()->Close();
		}

		if (ImGuiFileDialog::Instance()->Display("EDIT_LOG_FILE",
			ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking,
			minSize, maxSize))
		{
			if (ImGuiFileDialog::Instance()->IsOk())
			{
				auto& container_ref = LuaEngine::Instance()->GetSourceFilePathNamesRef();
				if (m_CurrentLogEdited > -1 && m_CurrentLogEdited < (int32_t)container_ref.size())
				{
					auto fpn = ImGuiFileDialog::Instance()->GetFilePathName();
					auto ps = ez::file::parsePathFileName(fpn);
					if (ps.isOk)
					{
						container_ref[m_CurrentLogEdited] = std::make_pair(ps.GetFPNE_WithPath(""), fpn);
						ProjectFile::Instance()->SetProjectChange();
					}
				}
			}

			ImGuiFileDialog::Instance()->Close();
		}
	}
    return false;
}

void ToolPane::UpdateTree()
{
	PrepareLogAfterSearch(ProjectFile::Instance()->m_SearchString);
}

void ToolPane::DrawTable()
{
	if (ImGui::CollapsingHeader("Lua Script File"))
	{
		if (ImGui::ContrastedButton("Select the Lua Script File", nullptr, nullptr, -1.0f, ImVec2(-1.0f, 0.0f)))
		{
            IGFD::FileDialogConfig config;
            config.countSelectionMax = 1;
            config.filePathName = LuaEngine::Instance()->GetLuaFilePathName();
            config.flags = ImGuiFileDialogFlags_Modal;
            ImGuiFileDialog::Instance()->OpenDialog("OPEN_LUA_SCRIPT_FILE", "Open a Lua Script File", ".lua,.*", config);
		}
		if (ImGui::ContrastedButton( "##LuaScriptEdit"))
		{
			ez::file::openFile(LuaEngine::Instance()->GetLuaFilePathName());
		}
		ImGui::SameLine();
		ImGui::TextWrapped("%s", LuaEngine::Instance()->GetLuaFileName().c_str());
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("%s", LuaEngine::Instance()->GetLuaFilePathName().c_str());
		}
	}

	if (ImGui::CollapsingHeader("Log Files"))
	{
		if (ImGui::ContrastedButton("Add a Log File", nullptr, nullptr, -1.0f, ImVec2(-1.0f, 0.0f))) {
            IGFD::FileDialogConfig config;
            config.countSelectionMax = 1;
            config.filePathName = ProjectFile::Instance()->m_LastLogFilePath;
            config.flags = ImGuiFileDialogFlags_Modal;
            ImGuiFileDialog::Instance()->OpenDialog("OPEN_LOG_FILE", "Open a Log File", ".*", config);
		}

		auto& container_ref = LuaEngine::Instance()->GetSourceFilePathNamesRef();

		if (!container_ref.empty())
		{
			static ImGuiTableFlags flags = ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders;
			if (ImGui::BeginTable("##sourcefilestable", 3, flags, ImVec2(-1.0f, container_ref.size() * ImGui::GetTextLineHeightWithSpacing())))
			{
				ImGui::TableSetupScrollFreeze(0, 1); // Make header always visible
				ImGui::TableSetupColumn("##Edit", ImGuiTableColumnFlags_WidthFixed);
				ImGui::TableSetupColumn("Log Files", ImGuiTableColumnFlags_WidthStretch);
				ImGui::TableSetupColumn("##Close", ImGuiTableColumnFlags_WidthFixed);
				ImGui::TableHeadersRow();

				int32_t idx = 0;
				auto it_to_edit = container_ref.end();
				auto it_to_erase = container_ref.end();
				for (auto it_source_file = container_ref.begin(); it_source_file != container_ref.end(); ++it_source_file)
				{
					ImGui::TableNextRow();

					if (ImGui::TableSetColumnIndex(0)) // second column
					{
						ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 1));
						if (ImGui::ContrastedButton( "##SourceFileEdit", nullptr, nullptr, 0.0f, ImVec2(16.0f, 16.0f)))
						{
							it_to_edit = it_source_file;
							m_CurrentLogEdited = idx;
						}
						ImGui::PopStyleVar();
					}

					if (ImGui::TableSetColumnIndex(1)) // first column
					{
						ImGui::Selectable(it_source_file->first.c_str());
						if (ImGui::IsItemHovered())
						{
							ImGui::SetTooltip("%s", it_source_file->second.c_str());
						}
					}

					if (ImGui::TableSetColumnIndex(2)) // second column
					{
						ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 1));
						if (ImGui::ContrastedButton(ICON_FONT_CANCEL "##SourceFileDelete", nullptr, nullptr, 0.0f, ImVec2(16.0f, 16.0f)))
						{
							it_to_erase = it_source_file;
						}
						ImGui::PopStyleVar();
					}

					++idx;
				}

				// edit
				if (it_to_edit != container_ref.end()) {
                    IGFD::FileDialogConfig config;
                    config.countSelectionMax = 1;
                    config.filePathName = it_to_edit->second;
                    config.flags = ImGuiFileDialogFlags_Modal;
                    ImGuiFileDialog::Instance()->OpenDialog("EDIT_LOG_FILE", "Edit a Log File", ".*", config);
				}

				// erase
				if (it_to_erase != container_ref.end())
				{
					container_ref.erase(it_to_erase);
				}

				ImGui::EndTable();
			}
		}
	}

	if (ImGui::CollapsingHeader("Predefined Zero value"))
	{
		ImGui::CheckBoxBoolDefault("Use Predefined Zero Value ?", &ProjectFile::Instance()->m_UsePredefinedZeroValue, false);
		if (ProjectFile::Instance()->m_UsePredefinedZeroValue)
		{
			ImGui::InputDouble("##Predefinedzerovalue", &ProjectFile::Instance()->m_PredefinedZeroValue);
		}
	}

	if (ImGui::CollapsingHeader("Analyse"))
	{
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
}

void ToolPane::DisplayItem(const SignalSerieWeak& vDatasSerie)
{
	if (!vDatasSerie.expired())
	{
		auto ptr = vDatasSerie.lock();
		if (ptr)
		{
			auto name_str = ez::str::toStr("%s (%u)", ptr->name.c_str(), (uint32_t)ptr->count_base_records);
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
		search_string = ez::str::toLower(m_search_buffer);
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

				auto cat_str = ez::str::toStr("%s (%u)", item_cat.first.c_str(), (uint32_t)item_cat.second.size());
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

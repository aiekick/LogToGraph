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

#include "SignalsHoveredList.h"
#include <Gui/MainFrame.h>
#include <ctools/cTools.h>
#include <ctools/Logger.h>
#include <ctools/FileHelper.h>
#include <Contrib/ImWidgets/ImWidgets.h>
#include <Project/ProjectFile.h>
#include <Project/ProjectFile.h>
#include <imgui/imgui_internal.h>
#include <Panes/Manager/LayoutManager.h>
#include <ImGuiFileDialog/ImGuiFileDialog.h>
#include <cinttypes> // printf zu
#include <Panes/LogPane.h>
#include <Panes/CodePane.h>
#include <Contrib/ImWidgets/ImWidgets.h>

#include <Engine/Lua/LuaEngine.h>
#include <Engine/Log/LogEngine.h>
#include <Engine/Log/SignalSerie.h>
#include <Engine/Log/SignalTick.h>

#include <Engine/Graphs/GraphView.h>

static int SourcePane_WidgetId = 0;

///////////////////////////////////////////////////////////////////////////////////
//// IMGUI PANE ///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////

bool SignalsHoveredList::Init()
{
	return true;
}

void SignalsHoveredList::Unit()
{

}

int SignalsHoveredList::DrawPanes(const uint32_t& /*vCurrentFrame*/, const int& vWidgetId, const std::string& /*vvUserDatas*/, PaneFlag& vInOutPaneShown)
{
	SourcePane_WidgetId = vWidgetId;

	if (vInOutPaneShown & m_PaneFlag)
	{
		static ImGuiWindowFlags flags =
			ImGuiWindowFlags_NoCollapse |
			ImGuiWindowFlags_NoBringToFrontOnFocus |
			ImGuiWindowFlags_MenuBar;
		if (ImGui::Begin<PaneFlag>(m_PaneName,
			&vInOutPaneShown , m_PaneFlag, flags))
		{
#ifdef USE_DECORATIONS_FOR_RESIZE_CHILD_WINDOWS
			auto win = ImGui::GetCurrentWindowRead();
			if (win->Viewport->Idx != 0)
				flags |= ImGuiWindowFlags_NoResize;
			else
				flags =	ImGuiWindowFlags_NoCollapse |
				ImGuiWindowFlags_NoBringToFrontOnFocus |
				ImGuiWindowFlags_MenuBar;
#endif
			if (ProjectFile::Instance()->IsLoaded())
			{
				DrawTable();
			}
		}

		ImGui::End();
	}

	return SourcePane_WidgetId;
}

void SignalsHoveredList::DrawDialogsAndPopups(const uint32_t& /*vCurrentFrame*/, const std::string& /*vvUserDatas*/)
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

int SignalsHoveredList::DrawWidgets(const uint32_t& /*vCurrentFrame*/, const int& vWidgetId, const std::string& /*vvUserDatas*/)
{
	return vWidgetId;
}

int SignalsHoveredList::CalcSignalsButtonCountAndSize(
	ImVec2& vOutCellSize,					/* cell size						*/
	ImVec2& vOutButtonSize)					/* button size (cell - paddings)	*/
{
	float aw = ImGui::GetContentRegionAvail().x;

	int count = ProjectFile::Instance()->m_SignalPreview_CountX;
	float width = ProjectFile::Instance()->m_SignalPreview_SizeX;

	count = (int)(aw / ct::maxi(width, 1.0f));
	width = aw / (float)ct::maxi(count, 1);

	ProjectFile::Instance()->m_SignalPreview_CountX = count;

	if (count > 0)
	{
		vOutCellSize = ImVec2(width, width);
		vOutButtonSize = vOutCellSize - ImGui::GetStyle().ItemSpacing - ImGui::GetStyle().FramePadding * 2.0f;
	}

	return count;
}

void SignalsHoveredList::DrawTable()
{
	auto win = ImGui::GetCurrentWindowRead();
	if (win)
	{
		const auto& signals_count = LogEngine::Instance()->GetPreviewTicks().size();
		if (signals_count)
		{
			static ImGuiTableFlags flags =
				ImGuiTableFlags_SizingFixedFit |
				ImGuiTableFlags_RowBg |
				ImGuiTableFlags_Hideable |
				ImGuiTableFlags_ScrollY |
				ImGuiTableFlags_NoHostExtendY;

			auto listViewID = ImGui::GetID("##SignalsHoveredList_DrawTable");
			if (ImGui::BeginTableEx("##SignalsHoveredList_DrawTable", listViewID, 5, flags)) //-V112
			{
				ImGui::TableSetupScrollFreeze(0, 1); // Make header always visible
				ImGui::TableSetupColumn("Epoch", ImGuiTableColumnFlags_WidthFixed);
				ImGui::TableSetupColumn("Date", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_DefaultHide);
				ImGui::TableSetupColumn("Cat", ImGuiTableColumnFlags_WidthFixed);
				ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
				ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthFixed);

				ImGui::TableHeadersRow();

				uint32_t count_color_push = 0U;
				ImU32 color = 0U;
				bool selected = false;
				m_VirtualClipper.Begin((int)signals_count, ImGui::GetTextLineHeightWithSpacing());
				while (m_VirtualClipper.Step())
				{
					for (int i = m_VirtualClipper.DisplayStart; i < m_VirtualClipper.DisplayEnd; ++i)
					{
						if (i < 0) continue;

						const auto infos_ptr = LogEngine::Instance()->GetPreviewTicks().at((size_t)i).lock();
						if (infos_ptr)
						{
							ImGui::TableNextRow();

							selected = LogEngine::Instance()->isSignalShown(infos_ptr->category, infos_ptr->name, &color);
							if (selected && color)
							{
								ImGui::PushStyleColor(ImGuiCol_Header, (ImU32)color);
								ImGui::PushStyleColor(ImGuiCol_HeaderActive, (ImU32)color);
								ImGui::PushStyleColor(ImGuiCol_HeaderHovered, (ImU32)color);
								count_color_push = 3U;
								if (ImGui::PushStyleColorWithContrast(ImGuiCol_Header, ImGuiCol_Text,
									ImGui::CustomStyle::Instance()->puContrastedTextColor,
									ImGui::CustomStyle::Instance()->puContrastRatio))
								{
									count_color_push = 4U;
								}
							}
							else
							{
								color = 0U;
							}
							
							if (ImGui::TableNextColumn()) // time
							{
								ImGui::Selectable(ct::toStr("%f", infos_ptr->time_epoch).c_str(), &selected, ImGuiSelectableFlags_SpanAllColumns);
							}
							if (ImGui::TableNextColumn()) // date time
							{
								ImGui::Selectable(infos_ptr->time_date_time.c_str(), &selected, ImGuiSelectableFlags_SpanAllColumns);
							}
							if (ImGui::TableNextColumn()) // category
							{
								ImGui::Selectable(infos_ptr->category.c_str(), &selected, ImGuiSelectableFlags_SpanAllColumns);
							}
							if (ImGui::TableNextColumn()) // name
							{
								ImGui::Selectable(infos_ptr->name.c_str(), &selected, ImGuiSelectableFlags_SpanAllColumns);
							}
							if (ImGui::TableNextColumn()) // value
							{
								ImGui::Text("%f", infos_ptr->value);
							}

							if (color)
							{
								ImGui::PopStyleColor(count_color_push);
							}
						}
					}
				}
				m_VirtualClipper.End();

				ImGui::EndTable();
			}
		}
	}
}
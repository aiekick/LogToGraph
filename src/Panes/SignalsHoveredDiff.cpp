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

#include "SignalsHoveredDiff.h"
#include <Gui/MainFrame.h>
#include <ctools/FileHelper.h>
#include <Contrib/ImWidgets/ImWidgets.h>
#include <Project/ProjectFile.h>
#include <imgui/imgui_internal.h>
#include <Panes/Manager/LayoutManager.h>
#include <ImGuiFileDialog/ImGuiFileDialog.h>
#include <cinttypes> // printf zu
#include <Panes/CodePane.h>

#include <Engine/Lua/LuaEngine.h>
#include <Engine/Log/LogEngine.h>
#include <Engine/Log/SignalSerie.h>
#include <Engine/Log/SignalTick.h>

static int SourcePane_WidgetId = 0;
static GraphColor s_DefaultGraphColors;

///////////////////////////////////////////////////////////////////////////////////
//// IMGUI PANE ///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////

bool SignalsHoveredDiff::Init()
{
	return true;
}

void SignalsHoveredDiff::Unit()
{

}

int SignalsHoveredDiff::DrawPanes(const uint32_t& /*vCurrentFrame*/, const int& vWidgetId, const std::string& /*vvUserDatas*/, PaneFlag& vInOutPaneShown)
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

void SignalsHoveredDiff::DrawDialogsAndPopups(const uint32_t& /*vCurrentFrame*/, const std::string& /*vvUserDatas*/)
{
}

int SignalsHoveredDiff::DrawWidgets(const uint32_t& /*vCurrentFrame*/, const int& vWidgetId, const std::string& /*vvUserDatas*/)
{
	return vWidgetId;
}

void SignalsHoveredDiff::CheckItem(const SignalTickPtr& vSignalTick)
{
	if (vSignalTick && ImGui::IsItemHovered())
	{
		if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
		{
			LogEngine::Instance()->ShowHideSignal(vSignalTick->category, vSignalTick->name);
			LogEngine::Instance()->UpdateVisibleSignalsColoring();
			ProjectFile::Instance()->SetProjectChange();
		}
	}
}

void SignalsHoveredDiff::DrawTable()
{
	auto win = ImGui::GetCurrentWindowRead();
	if (win)
	{
		if (ImGui::BeginMenuBar())
		{
			if (ImGui::BeginMenu("Graph mark Colors"))
			{
				if (ImGui::ContrastedButton("R##ResetBarFirstDiffMarkColor"))
				{
					ProjectFile::Instance()->m_GraphColors.graphFirstDiffMarkColor =
						s_DefaultGraphColors.graphFirstDiffMarkColor;
				}

				ImGui::SameLine();

				ImGui::ColorEdit4("First diff mark color##ResetBarFirstDiffMarkColor",
					&ProjectFile::Instance()->m_GraphColors.graphFirstDiffMarkColor.x, ImGuiColorEditFlags_NoInputs);

				if (ImGui::ContrastedButton("R##ResetBarSecondDiffMarkColor"))
				{
					ProjectFile::Instance()->m_GraphColors.graphSecondDiffMarkColor =
						s_DefaultGraphColors.graphSecondDiffMarkColor;
				}

				ImGui::SameLine();

				ImGui::ColorEdit4("Second diff mark color##ResetBarSecondDiffMarkColor",
					&ProjectFile::Instance()->m_GraphColors.graphSecondDiffMarkColor.x, ImGuiColorEditFlags_NoInputs);

				ImGui::EndMenu();
			}

			ImGui::Text("Diff (?)");
			if (ImGui::IsItemHovered())
			{
				ImGui::SetTooltip("Diff in graph :\npress key 'f' for first tick\npress key 's' for second tick\npress key 'r' for reset diff marks");
			}

			ImGui::EndMenuBar();
		}

		const auto& signals_count = LogEngine::Instance()->GetDiffResultTicks().size();
		if (signals_count)
		{
			static ImGuiTableFlags flags =
				ImGuiTableFlags_SizingFixedFit |
				ImGuiTableFlags_RowBg |
				ImGuiTableFlags_Hideable |
				ImGuiTableFlags_ScrollY |
				ImGuiTableFlags_NoHostExtendY;

			auto listViewID = ImGui::GetID("##SignalsHoveredDiff_DrawTable");
			if (ImGui::BeginTableEx("##SignalsHoveredDiff_DrawTable", listViewID, 4, flags)) //-V112
			{
				ImGui::TableSetupScrollFreeze(0, 1); // Make header always visible
				ImGui::TableSetupColumn("Cat", ImGuiTableColumnFlags_WidthFixed);
				ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
				ImGui::TableSetupColumn("First Value", ImGuiTableColumnFlags_WidthFixed);
				ImGui::TableSetupColumn("Second Value", ImGuiTableColumnFlags_WidthFixed);

				ImGui::TableHeadersRow();

				int32_t count_color_push = 0;
				ImU32 color = 0U;
				bool selected = false;
				m_VirtualClipper.Begin((int)signals_count, ImGui::GetTextLineHeightWithSpacing());
				while (m_VirtualClipper.Step())
				{
					for (int i = m_VirtualClipper.DisplayStart; i < m_VirtualClipper.DisplayEnd; ++i)
					{
						if (i < 0) continue;

						const auto& diff_item = LogEngine::Instance()->GetDiffResultTicks().at((size_t)i);
						const auto& diff_first_mark_ptr = diff_item.first.lock();
						const auto& diff_second_mark_ptr = diff_item.second.lock();
						if (diff_first_mark_ptr && 
							diff_second_mark_ptr)
						{
							ImGui::TableNextRow();

							selected = LogEngine::Instance()->isSignalShown(diff_first_mark_ptr->category, diff_first_mark_ptr->name, &color);
							if (selected && color)
							{
								ImGui::PushStyleColor(ImGuiCol_Header, (ImU32)color);
								ImGui::PushStyleColor(ImGuiCol_HeaderActive, (ImU32)color);
								ImGui::PushStyleColor(ImGuiCol_HeaderHovered, (ImU32)color);
								count_color_push = 3;
								if (ImGui::PushStyleColorWithContrast(ImGuiCol_Header, ImGuiCol_Text,
									ImGui::CustomStyle::Instance()->puContrastedTextColor,
									ImGui::CustomStyle::Instance()->puContrastRatio))
								{
									count_color_push = 4;
								}
							}
							else
							{
								color = 0U;
							}

							if (ImGui::TableNextColumn()) // category
							{
								ImGui::Selectable(diff_first_mark_ptr->category.c_str(), &selected, ImGuiSelectableFlags_SpanAllColumns);
								CheckItem(diff_first_mark_ptr);
							}
							if (ImGui::TableNextColumn()) // name
							{
								ImGui::Selectable(diff_first_mark_ptr->name.c_str(), &selected, ImGuiSelectableFlags_SpanAllColumns);
								CheckItem(diff_first_mark_ptr);
							}
							if (ImGui::TableNextColumn()) // first value
							{
								ImGui::Text("%f", diff_first_mark_ptr->value);
								CheckItem(diff_first_mark_ptr);
							}
							if (ImGui::TableNextColumn()) // second value
							{
								if (diff_second_mark_ptr->string.empty())
								{
									ImGui::Text("%f", diff_second_mark_ptr->value);
								}
								else
								{
									ImGui::Text("%s", diff_second_mark_ptr->string.c_str());
								}
								CheckItem(diff_second_mark_ptr);
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
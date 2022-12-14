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

#include "LogPaneSecondView.h"
#include <Gui/MainFrame.h>
#include <ctools/cTools.h>
#include <ctools/Logger.h>
#include <Panes/ToolPane.h>
#include <Panes/SignalsHoveredMap.h>
#include <Panes/GraphGroupPane.h>
#include <Panes/GraphListPane.h>
#include <Helper/Messaging.h>
#include <Project/ProjectFile.h>
#include <imgui/imgui_internal.h>
#include <Panes/Manager/LayoutManager.h>
#include <Contrib/ImWidgets/ImWidgets.h>
#include <cinttypes> // printf zu

#include <Engine/Log/LogEngine.h>
#include <Engine/Log/SignalSerie.h>
#include <Engine/Log/SignalTick.h>
#include <Engine/Lua/LuaEngine.h>

static int GeneratorPaneWidgetId = 0;

///////////////////////////////////////////////////////////////////////////////////
//// OVERRIDES ////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////

bool LogPaneSecondView::Init()
{
	return true;
}

void LogPaneSecondView::Unit()
{

}

int LogPaneSecondView::DrawPanes(const uint32_t& /*vCurrentFrame*/, const int& vWidgetId, const std::string& /*vvUserDatas*/, PaneFlag& vInOutPaneShown)
{
	GeneratorPaneWidgetId = vWidgetId;

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
				flags |= ImGuiWindowFlags_NoResize;// | ImGuiWindowFlags_NoTitleBar;
			else
				flags = ImGuiWindowFlags_NoCollapse |
				ImGuiWindowFlags_NoBringToFrontOnFocus |
				ImGuiWindowFlags_MenuBar;
#endif
			if (ProjectFile::Instance()->IsLoaded()) 
			{
				DrawTable();
			}
		}

		//MainFrame::sAnyWindowsHovered |= ImGui::IsWindowHovered();

		ImGui::End();
	}

	return GeneratorPaneWidgetId;
}

void LogPaneSecondView::DrawDialogsAndPopups(const uint32_t& /*vCurrentFrame*/, const std::string& /*vvUserDatas*/)
{
	/*ImVec2 min = MainFrame::Instance()->puDisplaySize * 0.5f;
	ImVec2 max = MainFrame::Instance()->puDisplaySize;

	if (ImGuiFileDialog::Instance()->Display("GenerateFileDlg", ImGuiWindowFlags_NoDocking, min, max))
	{
		if (ImGuiFileDialog::Instance()->IsOk())
		{
			std::string filePath = ImGuiFileDialog::Instance()->GetCurrentPath();
			std::string fileName = ImGuiFileDialog::Instance()->GetCurrentFileName();
			Generator::Instance()->Generate(filePath, fileName, vProjectFile);
		}

		ImGuiFileDialog::Instance()->CloseDialog("GenerateFileDlg");
	}*/
}

int LogPaneSecondView::DrawWidgets(const uint32_t& /*vCurrentFrame*/, const int& vWidgetId, const std::string& /*vvUserDatas*/)
{
	return vWidgetId;
}

std::string LogPaneSecondView::getXml(const std::string& /*vOffset*/, const std::string& /*vUserDatas*/)
{
	std::string str;

	return str;
}

bool LogPaneSecondView::setFromXml(tinyxml2::XMLElement* vElem, tinyxml2::XMLElement* vParent, const std::string& /*vUserDatas*/)
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

	return true;
}

void LogPaneSecondView::Clear()
{
	m_LogDatas.clear();
}

void LogPaneSecondView::CheckItem(SignalTickPtr vSignalTick)
{
	if (vSignalTick && ImGui::IsItemHovered())
	{
		LogEngine::Instance()->SetHoveredTime(vSignalTick->time_epoch);

		if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
		{
			LogEngine::Instance()->ShowHideSignal(vSignalTick->category, vSignalTick->name);
			ProjectFile::Instance()->SetProjectChange();
			ToolPane::Instance()->UpdateTree();
			GraphListPane::Instance()->UpdateDB();

			m_need_re_preparation = true;
		}

		// first mark
		if (ImGui::IsKeyPressed(ImGuiKey_F))
		{
			LogEngine::Instance()->SetFirstDiffMark(vSignalTick->time_epoch);
		}

		// second mark
		if (ImGui::IsKeyPressed(ImGuiKey_S))
		{
			LogEngine::Instance()->SetSecondDiffMark(vSignalTick->time_epoch);
		}

		// second mark
		if (ImGui::IsKeyPressed(ImGuiKey_R))
		{
			LogEngine::Instance()->SetFirstDiffMark(0.0);
			LogEngine::Instance()->SetSecondDiffMark(0.0);
		}
	}
}

void LogPaneSecondView::DrawTable()
{
	if (ImGui::BeginMenuBar())
	{
		bool need_update = ImGui::Checkbox("Collapse Selection", &ProjectFile::Instance()->m_CollapseLogSelection);
		need_update |= ImGui::Checkbox("Hide some values", &ProjectFile::Instance()->m_HideSomeValues);

		if (ProjectFile::Instance()->m_HideSomeValues)
		{
			ImGui::Text("(?)");
			if (ImGui::IsItemHovered())
			{
				ImGui::SetTooltip("%s", "you can define many values, ex : 1,2,3.2,5.8");
			}

			if (ImGui::ContrastedButton("R##ResetLogPaneSecondViewTable"))
			{
				ProjectFile::Instance()->m_ValuesToHide.clear();
				need_update = true;
			}

			static char _values_hide_buffer[1024 + 1] = "";
			snprintf(_values_hide_buffer, 1024, "%s", ProjectFile::Instance()->m_ValuesToHide.c_str());
			if (ImGui::InputText("##Valuestohide", _values_hide_buffer, 1024))
			{
				need_update = true;
				ProjectFile::Instance()->m_ValuesToHide = _values_hide_buffer;
			}
		}

		if (need_update)
		{
			PrepareLog();

			ProjectFile::Instance()->SetProjectChange();
		}

		ImGui::EndMenuBar();
	}

	static ImGuiTableFlags flags =
		ImGuiTableFlags_SizingFixedFit | 
		ImGuiTableFlags_RowBg |
		ImGuiTableFlags_Hideable | 
		ImGuiTableFlags_ScrollY | 
		//ImGuiTableFlags_Resizable |
		ImGuiTableFlags_NoHostExtendY;

	// first display
	if (m_LogDatas.empty())
	{
		PrepareLog();
	}

	const auto _count_logs = m_LogDatas.size();

	m_need_re_preparation = false;

	auto listViewID = ImGui::GetID("##LogPaneSecondView_DrawTable");
	if (ImGui::BeginTableEx("##LogPaneSecondView_DrawTable", listViewID, 5, flags)) //-V112
	{
		ImGui::TableSetupScrollFreeze(0, 1); // Make header always visible
		ImGui::TableSetupColumn("Epoch", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_DefaultHide);
		ImGui::TableSetupColumn("Date", ImGuiTableColumnFlags_WidthFixed);
		ImGui::TableSetupColumn("Cat", ImGuiTableColumnFlags_WidthFixed);
		ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthFixed);

		ImGui::TableNextRow(ImGuiTableRowFlags_Headers);

		for (int column = 0; column < 5; column++) //-V112
		{
			ImGui::TableSetColumnIndex(column);
			const char* column_name = ImGui::TableGetColumnName(column); // Retrieve name passed to TableSetupColumn()
			ImGui::PushID(column);
			ImGui::TableHeader(column_name);
			ImGui::PopID();
		}

		uint32_t count_color_push = 0U;
		ImU32 color = 0U;
		bool selected = false;
		m_LogListClipper.Begin((int)_count_logs, ImGui::GetTextLineHeightWithSpacing());
		while (m_LogListClipper.Step())
		{
			for (int i = m_LogListClipper.DisplayStart; i < m_LogListClipper.DisplayEnd; ++i)
			{
				if (i < 0) continue;

				const auto infos_ptr = m_LogDatas.at((size_t)i).lock();
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
						CheckItem(infos_ptr);
					}
					if (ImGui::TableNextColumn()) // date time
					{
						ImGui::Selectable(infos_ptr->time_date_time.c_str(), &selected, ImGuiSelectableFlags_SpanAllColumns);
						CheckItem(infos_ptr);
					}
					if (ImGui::TableNextColumn()) // category
					{
						ImGui::Selectable(infos_ptr->category.c_str(), &selected, ImGuiSelectableFlags_SpanAllColumns);
						CheckItem(infos_ptr);
					}
					if (ImGui::TableNextColumn()) // name
					{
						ImGui::Selectable(infos_ptr->name.c_str(), &selected, ImGuiSelectableFlags_SpanAllColumns);
						CheckItem(infos_ptr);
					}
					if (ImGui::TableNextColumn()) // value
					{
						ImGui::Text("%f", infos_ptr->value);
						CheckItem(infos_ptr);
					}

					if (color)
					{
						ImGui::PopStyleColor(count_color_push);
					}
				}
			}
		}
		m_LogListClipper.End();

		ImGui::EndTable();
	}

	if (m_need_re_preparation)
	{
		PrepareLog();
	}
}

void LogPaneSecondView::PrepareLog()
{
	if (LuaEngine::Instance()->IsJoinable())
		return;

	m_LogDatas.clear();

	if (ProjectFile::Instance()->m_HideSomeValues)
	{
		m_ValuesToHide.clear();
		auto arr = ct::splitStringToVector(ProjectFile::Instance()->m_ValuesToHide, ",");
		for (const auto& a : arr)
		{
			m_ValuesToHide.push_back(ct::dvariant(a).GetD());
		}
	}

	const auto _count_logs = LogEngine::Instance()->GetSignalTicks().size();
	const auto _collapseSelection = ProjectFile::Instance()->m_CollapseLogSelection;

	for (size_t idx = 0U; idx < _count_logs; ++idx)
	{
		const auto& infos_ptr = LogEngine::Instance()->GetSignalTicks().at(idx);
		if (infos_ptr)
		{
			auto selected = LogEngine::Instance()->isSignalShown(infos_ptr->category, infos_ptr->name);
			if (_collapseSelection && !selected)
				continue;

			if (ProjectFile::Instance()->m_HideSomeValues)
			{
				bool found = false;

				for (const auto& a : m_ValuesToHide)
				{
					if (IS_DOUBLE_EQUAL(a, infos_ptr->value))
					{
						found = true;
						break;
					}
				}

				if (found)
				{
					continue;
				}
			}

			m_LogDatas.push_back(infos_ptr);
		}
	}
}
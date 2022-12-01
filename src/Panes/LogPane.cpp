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

#include "LogPane.h"
#include <Gui/MainFrame.h>
#include <ctools/cTools.h>
#include <ctools/Logger.h>
#include <Panes/ToolPane.h>
#include <Helper/Messaging.h>
#include <Project/ProjectFile.h>
#include <imgui/imgui_internal.h>
#include <Panes/Manager/LayoutManager.h>
#include <Contrib/ImWidgets/ImWidgets.h>
#include <cinttypes> // printf zu

static int GeneratorPaneWidgetId = 0;

///////////////////////////////////////////////////////////////////////////////////
//// OVERRIDES ////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////

bool LogPane::Init()
{
	return true;
}

void LogPane::Unit()
{

}

int LogPane::DrawPanes(const uint32_t& /*vCurrentFrame*/, int vWidgetId, std::string /*vUserDatas*/, PaneFlags& vInOutPaneShown)
{
	GeneratorPaneWidgetId = vWidgetId;

	if (vInOutPaneShown & m_PaneFlag)
	{
		static ImGuiWindowFlags flags =
			ImGuiWindowFlags_NoCollapse |
			ImGuiWindowFlags_NoBringToFrontOnFocus |
			ImGuiWindowFlags_MenuBar;
		if (ImGui::Begin<PaneFlags>(m_PaneName,
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

void LogPane::DrawDialogsAndPopups(const uint32_t& /*vCurrentFrame*/, std::string /*vUserDatas*/)
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

int LogPane::DrawWidgets(const uint32_t& /*vCurrentFrame*/, int vWidgetId, std::string /*vUserDatas*/)
{
	return vWidgetId;
}

std::string LogPane::getXml(const std::string& /*vOffset*/, const std::string& /*vUserDatas*/)
{
	std::string str;

	return str;
}

bool LogPane::setFromXml(tinyxml2::XMLElement* vElem, tinyxml2::XMLElement* vParent, const std::string& /*vUserDatas*/)
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

void LogPane::DrawTable()
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

			if (ImGui::ContrastedButton("R##ResetLogPaneTable"))
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
		ImGuiTableFlags_NoHostExtendY | 
		ImGuiTableFlags_Resizable;

	// first display
	if (m_LogDatas.empty())
	{
		PrepareLog();
	}

	const auto _count_logs = m_LogDatas.size();

	bool _need_re_preparation = false;

	auto listViewID = ImGui::GetID("##LogPane_DrawTable");
	if (ImGui::BeginTableEx("##LogPane_DrawTable", listViewID, 5, flags)) //-V112
	{
		ImGui::TableSetupScrollFreeze(0, 1); // Make header always visible
		ImGui::TableSetupColumn("Epoch Time", ImGuiTableColumnFlags_WidthStretch, -1, 0);
		ImGui::TableSetupColumn("Date Time", ImGuiTableColumnFlags_WidthStretch, -1, 1);
		ImGui::TableSetupColumn("Category", ImGuiTableColumnFlags_WidthStretch, -1, 2);
		ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch, -1, 3);
		ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch, -1, 4);

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

				const auto& infos = m_LogDatas.at((size_t)i);

				ImGui::TableNextRow();

				selected = LogEngine::Instance()->isSignalShown(infos.category, infos.name, &color);
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
					ImGui::Selectable(ct::toStr("%f", infos.time_epoch).c_str(), &selected, ImGuiSelectableFlags_SpanAllColumns);

					if (ImGui::IsItemHovered())
					{
						LogEngine::Instance()->SetHoveredTime(infos.time_epoch);

						if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
						{
							LogEngine::Instance()->ShowHideSignal(infos.category, infos.name);
							ProjectFile::Instance()->SetProjectChange();
							ToolPane::Instance()->UpdateTree();

							_need_re_preparation = true;
						}
					}
				}
				if (ImGui::TableNextColumn()) // date time
				{
					ImGui::Selectable(infos.time_date_time.c_str(), &selected, ImGuiSelectableFlags_SpanAllColumns);
				}
				if (ImGui::TableNextColumn()) // category
				{
					ImGui::Selectable(infos.category.c_str(), &selected, ImGuiSelectableFlags_SpanAllColumns);
				}
				if (ImGui::TableNextColumn()) // name
				{
					ImGui::Selectable(infos.name.c_str(), &selected, ImGuiSelectableFlags_SpanAllColumns);
				}
				if (ImGui::TableNextColumn()) // value
				{
					ImGui::Text("%f", infos.value);
				}

				if (color)
				{
					ImGui::PopStyleColor(count_color_push);
				}
			}
		}
		m_LogListClipper.End();
	}

	ImGui::EndTable();

	if (_need_re_preparation)
	{
		PrepareLog();
	}
}

void LogPane::PrepareLog()
{
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

	const auto _count_logs = LogEngine::Instance()->logDatasSize();
	const auto _collapseSelection = ProjectFile::Instance()->m_CollapseLogSelection;

	for (size_t idx = 0U; idx < _count_logs; ++idx)
	{
		const auto& infos = LogEngine::Instance()->at(idx);

		auto selected = LogEngine::Instance()->isSignalShown(infos.category, infos.name);
		if (_collapseSelection && !selected)
			continue;

		if (ProjectFile::Instance()->m_HideSomeValues)
		{
			bool found = false;

			for (const auto& a : m_ValuesToHide)
			{
				if (IS_DOUBLE_EQUAL(a, infos.value))
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

		m_LogDatas.push_back(infos);
	}
}
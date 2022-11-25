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

#include "GraphPane.h"
#include <Gui/MainFrame.h>
#include <ctools/cTools.h>
#include <ctools/Logger.h>
#include <Helper/Messaging.h>
#include <Project/ProjectFile.h>
#include <Panes/Manager/LayoutManager.h>
#include <Contrib/ImWidgets/ImWidgets.h>

#include <implot/implot.h>
#include <implot/implot_internal.h>
#include <imgui/imgui_internal.h>

#include <Engine/Log/LogEngine.h>

#include <cinttypes> // printf zu

static int GeneratorPaneWidgetId = 0;

///////////////////////////////////////////////////////////////////////////////////
//// OVERRIDES ////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////

bool GraphPane::Init()
{
	return true;
}

void GraphPane::Unit()
{

}

int GraphPane::DrawPanes(const uint32_t& /*vCurrentFrame*/, int vWidgetId, std::string /*vUserDatas*/, PaneFlags& vInOutPaneShown)
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
				DrawGraph();
			}
		}

		//MainFrame::sAnyWindowsHovered |= ImGui::IsWindowHovered();

		ImGui::End();
	}

	return GeneratorPaneWidgetId;
}

void GraphPane::DrawDialogsAndPopups(const uint32_t& /*vCurrentFrame*/, std::string /*vUserDatas*/)
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

int GraphPane::DrawWidgets(const uint32_t& /*vCurrentFrame*/, int vWidgetId, std::string /*vUserDatas*/)
{
	return vWidgetId;
}

std::string GraphPane::getXml(const std::string& /*vOffset*/, const std::string& /*vUserDatas*/)
{
	std::string str;

	return str;
}

bool GraphPane::setFromXml(tinyxml2::XMLElement* vElem, tinyxml2::XMLElement* vParent, const std::string& /*vUserDatas*/)
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

template <typename T>
int BinarySearch(const T* arr, int l, int r, T x) 
{
	if (r >= l) 
	{
		int mid = l + (r - l) / 2;
		if (arr[mid] == x)
			return mid;
		if (arr[mid] > x)
			return BinarySearch(arr, l, mid - 1, x);
		return BinarySearch(arr, mid + 1, r, x);
	}
	return -1;
}

void GraphPane::DrawGraph()
{
	bool _need_show_hide_x_axis = false;
	bool _need_show_hide_y_axis = false;

	if (ImGui::BeginMenuBar())
	{
		if (ImGui::BeginMenu("Colors"))
		{
			if (ImGui::ContrastedButton("R##ResetBarColor"))
			{
				ProjectFile::Instance()->m_GraphBarColor =
					ProjectFile::Instance()->m_DefaultGraphBarColor;
			}

			ImGui::SameLine();

			ImGui::ColorEdit4("Bars Color##tBarColor",
				&ProjectFile::Instance()->m_GraphBarColor.x, ImGuiColorEditFlags_NoInputs);


			if (ImGui::ContrastedButton("R##ResetHoveredTimeBarColor"))
			{
				ProjectFile::Instance()->m_GraphHoveredTimeColor =
					ProjectFile::Instance()->m_DefaultGraphHoveredTimeColor;
			}

			ImGui::SameLine();

			ImGui::ColorEdit4("Current Time Color##HoveredTimeBarColor",
				&ProjectFile::Instance()->m_GraphHoveredTimeColor.x, ImGuiColorEditFlags_NoInputs);

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Axis Labels"))
		{
			if (ImGui::MenuItem(m_show_hide_y_axis ? "Show X Axis LabelsR##GraphPaneDrawPanes" : "Hide X Axis LabelsR##GraphPaneDrawPanes"))
			{
				m_show_hide_x_axis = !m_show_hide_x_axis;
				_need_show_hide_x_axis = true;
			}

			if (ImGui::MenuItem(m_show_hide_y_axis ? "Show Y Axis LabelsR##GraphPaneDrawPanes" : "Hide Y Axis LabelsR##GraphPaneDrawPanes"))
			{
				m_show_hide_y_axis = !m_show_hide_y_axis;
				_need_show_hide_y_axis = true;
			}

			ImGui::EndMenu();
		}

		ImGui::EndMenuBar();
	}

	ImPlot::GetStyle().UseLocalTime = false;
	ImPlot::GetStyle().UseISO8601 = false;
	ImPlot::GetStyle().Use24HourClock = false;

	const ImU32 color_bars = ImGui::GetColorU32(ProjectFile::Instance()->m_GraphBarColor);
	const ImU32 color_green = ImGui::GetColorU32(ProjectFile::Instance()->m_GraphHoveredTimeColor);
	ImVec2 last_value_pos, value_pos, hovered_min_pos, hovered_max_pos;
	for (const auto& item_cat : *LogEngine::Instance())
	{
		for (const auto& item_name : item_cat.second)
		{
			const auto& datas = item_name.second;
			if (datas.show)
			{
				const auto& name_str = item_cat.first + " / " + item_name.first;

				if (ImPlot::BeginPlot(name_str.c_str(), ImVec2(-1, 150), ImPlotFlags_NoLegend/* | ImPlotFlags_Crosshairs*/))
				{
					if (_need_show_hide_x_axis)
					{
						ImPlotPlot& plot = *GImPlot->CurrentPlot;
						ImPlotAxis& axis = plot.Axes[ImAxis_X1];
						ImFlipFlag(axis.Flags, ImPlotAxisFlags_NoTickLabels);
					}

					if (_need_show_hide_y_axis)
					{
						ImPlotPlot& plot = *GImPlot->CurrentPlot;
						ImPlotAxis& axis = plot.Axes[ImAxis_Y1];
						ImFlipFlag(axis.Flags, ImPlotAxisFlags_NoTickLabels);
					}

					ImPlot::SetupAxes(NULL, NULL, ImPlotAxisFlags_NoTickLabels, ImPlotAxisFlags_AutoFit | ImPlotAxisFlags_RangeFit);

					ImPlot::SetupAxesLimits(datas.min_date, datas.max_date, datas.min_value, datas.max_value);

					ImPlot::SetupAxisScale(ImAxis_X1, ImPlotScale_Time);

					ImPlot::SetupAxisLimitsConstraints(ImAxis_X1, datas.min_date, datas.max_date);
					ImPlot::SetupAxisFormat(ImAxis_Y1, "%.2f");

					ImDrawList* draw_list = ImPlot::GetPlotDrawList();

					if (ImPlot::BeginItem(item_name.first.c_str()))
					{
						ImPlot::GetCurrentItem()->Color = IM_COL32(64, 64, 64, 255);
						if (ImPlot::FitThisFrame())
						{
							for (size_t i = 0; i < datas.datas_values.size(); ++i)
							{
								ImPlot::FitPoint(ImPlotPoint(datas.datas_times.at(i), datas.datas_values.at(i)));
							}
						}

						double hovered_time = LogEngine::Instance()->GetHoveredTime();

						// render data
						if (datas.datas_values.size() > 0U)
						{
							float zero_y = (float)ImPlot::PlotToPixels(0.0, 0.0).y;
							double last_time = datas.datas_times.at(0U), current_time;
							last_value_pos = ImPlot::PlotToPixels(last_time, datas.datas_values.at(0U));
							for (size_t i = 1U; i < datas.datas_values.size(); ++i)
							{
								current_time = datas.datas_times.at(i);
								value_pos = ImPlot::PlotToPixels(current_time, datas.datas_values.at(i));
								draw_list->AddRectFilled(ImVec2(last_value_pos.x, zero_y), ImVec2(value_pos.x, last_value_pos.y), color_bars);

								if (hovered_time >= last_time && hovered_time <= current_time)
								{
									hovered_min_pos = ImPlot::PlotToPixels(hovered_time, datas.min_value);
									hovered_max_pos = ImPlot::PlotToPixels(hovered_time, datas.max_value);
									draw_list->AddLine(hovered_min_pos, hovered_max_pos, color_green, 5.0f);
								}

								last_value_pos = value_pos;
								last_time = current_time;
							}
						}

						ImPlot::EndItem();
					}

					ImPlot::EndPlot();
				}
			}
		}
	}
}

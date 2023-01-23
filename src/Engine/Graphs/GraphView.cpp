// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

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

#include "GraphView.h"

#include <implot/implot_internal.h>

#include <Contrib/ImWidgets/ImWidgets.h>

#include <imgui/imgui_internal.h>

#include <Engine/Log/LogEngine.h>
#include <Engine/Log/SignalSerie.h>
#include <Engine/Log/SignalTick.h>

#include <Project/ProjectFile.h>

#include <Engine/Graphs/GraphGroup.h>
#include <Panes/Manager/LayoutManager.h>

#include <Panes/LogPane.h>

#define DRAG_LINE_LOG_HOVERED_TIME 0
#define DRAG_LINE_FIRST_DIFF_MARK 1
#define DRAG_LINE_SECOND_DIFF_MARK 2
#define DRAG_LINE_MOUSE_HOVERED_TIME 2

static GraphColor s_DefaultGraphColors;

// todo : to put in a common file
// https://www.shadertoy.com/view/ld3fzf
ct::fvec4 GraphView::GetRainBow(const int32_t& vIdx, const int32_t& vCount)
{
	float r = (float)(vIdx + 1U) / (float)vCount;
	auto c = ct::cos(ct::fvec4(0.0f, 23.0f, 21.0f, 1.0f) + r * 6.3f) * 0.5f + 0.5f;
	c.w = 0.75f;
	return c;
}

GraphView::GraphView()
{
	Clear();
}

void GraphView::Clear()
{
	m_GraphGroups.clear(); 
	m_GraphGroups.push_back(GraphGroup::Create()); // first group : default group
	m_GraphGroups.push_back(GraphGroup::Create()); // last group
	m_Range_Value = SignalValueRange(0.5, -0.5) * DBL_MAX;
}

void GraphView::AddSerieToGroup(const SignalSerieWeak& vSignalSerie, const GraphGroupPtr& vToGroupPtr)
{
	auto ptr = vSignalSerie.lock();
	if (ptr && vToGroupPtr)
	{
		// if this is the last, the last group will become a new group
		if (vToGroupPtr == m_GraphGroups.back())
		{
			auto _ptr = GraphGroup::Create();
			if (_ptr)
			{
				m_GraphGroups.push_back(_ptr);
			}
		}

		ptr->graph_groupd_ptr = vToGroupPtr;

		vToGroupPtr->AddSignalSerie(vSignalSerie);

		m_Range_Value.x = ct::mini(m_Range_Value.x, vToGroupPtr->GetSignalSeriesRange().x);
		m_Range_Value.y = ct::maxi(m_Range_Value.y, vToGroupPtr->GetSignalSeriesRange().y);
	}
}

void GraphView::AddSerieToGroupID(const SignalSerieWeak& vSignalSerie, const size_t& vToGroupID)
{
	auto ptr = vSignalSerie.lock();
	if (ptr)
	{
		while (m_GraphGroups.size() <= vToGroupID + 1) // +1 because always one more than the current max
		{
			auto _ptr = GraphGroup::Create();
			if (_ptr)
			{
				m_GraphGroups.push_back(_ptr);
			}
		}

		auto group_ptr = prGetGroupAt(vToGroupID);
		if (group_ptr)
		{
			ptr->graph_groupd_ptr = group_ptr;

			group_ptr->AddSignalSerie(vSignalSerie);

			m_Range_Value.x = ct::mini(m_Range_Value.x, group_ptr->GetSignalSeriesRange().x);
			m_Range_Value.y = ct::maxi(m_Range_Value.y, group_ptr->GetSignalSeriesRange().y);

			ComputeGraphsCount();
		}
	}
}

void GraphView::AddSerieToDefaultGroup(const SignalSerieWeak& vSignalSerie)
{
	AddSerieToGroup(vSignalSerie, m_GraphGroups.front());
}

void GraphView::RemoveSerieFromGroup(const SignalSerieWeak& vSignalSerie, const GraphGroupPtr& vFromGroupPtr)
{
	if (vFromGroupPtr)
	{
		vFromGroupPtr->RemoveSignalSerie(vSignalSerie);

		m_Range_Value.x = ct::mini(m_Range_Value.x, vFromGroupPtr->GetSignalSeriesRange().x);
		m_Range_Value.y = ct::maxi(m_Range_Value.y, vFromGroupPtr->GetSignalSeriesRange().y);
	}
}

void GraphView::RemoveEmptyGroups()
{
	if (m_GraphGroups.size() > 2U)
	{
		std::list<size_t> group_to_erase;

		// first pass : we will save the group id of empty groups
		size_t idx = 0U;
		for (const auto& group_ptr : m_GraphGroups)
		{
			if (group_ptr &&							// valid group ptr
				group_ptr != m_GraphGroups.front() &&	// not the first group (must always be selectable, so we keep it)
				group_ptr != m_GraphGroups.back() &&	// not the last group (must always be selectable, so we keep it)
				group_ptr->GetSignalSeries().empty())	// no signals series
			{
				// we insert in front
				// like that we will erase in inverse order
				// because if we delete e index 1 before a 2
				// after the 2 is in fact the 1 and we can have an issue
				// if we delete the 2 before the 1, all is ok
				group_to_erase.push_front(idx);
			}

			++idx;
		}

		// second pass : we will delete the found group ids
		for (const auto& group_id : group_to_erase)
		{
			prEraseGroupAt(group_id);
		}
	}

	ComputeGraphsCount();
}

void GraphView::MoveSerieFromGroupToGroup(const SignalSerieWeak& vSignalSerie, const GraphGroupPtr& vFromGroupPtr, const GraphGroupPtr& vToGroupPtr)
{
	RemoveSerieFromGroup(vSignalSerie, vFromGroupPtr);
	AddSerieToGroup(vSignalSerie, vToGroupPtr);
	RemoveEmptyGroups();
}

size_t GraphView::GetGroupID(const GraphGroupPtr& vToGroupPtr) const
{
	size_t idx = 0U;
	for (const auto& group_ptr : m_GraphGroups)
	{
		if (group_ptr == vToGroupPtr)
		{
			break;
		}

		++idx;
	}

	return idx;
}

GraphGroupsRef GraphView::GetGraphGroups()
{
	return m_GraphGroups;
}

void GraphView::DrawGraphGroupTable()
{
	if (ImGui::BeginMenuBar())
	{
		ImGui::MenuItem("ReColorize (Rainbow)", nullptr, &ProjectFile::Instance()->m_AutoColorize);

		ImGui::EndMenuBar();
	}

	static ImGuiTableFlags flags =
		ImGuiTableFlags_SizingFixedFit |
		ImGuiTableFlags_RowBg |
		ImGuiTableFlags_ScrollY |
		ImGuiTableFlags_NoHostExtendY;

	const auto _column_count = (int32_t)m_GraphGroups.size();
	auto listViewID = ImGui::GetID("##GraphView_DrawGraphGroupTable");
	if (ImGui::BeginTableEx("##GraphView_DrawGraphGroupTable", listViewID, 2U + _column_count, flags)) //-V112
	{
		ImGui::TableSetupScrollFreeze(0, 1); // Make header always visible
		ImGui::TableSetupColumn("Col", ImGuiTableColumnFlags_WidthFixed, -1);
		ImGui::TableSetupColumn("Signal", ImGuiTableColumnFlags_WidthStretch, -1);
		ImGui::TableSetupColumn("GDef", ImGuiTableColumnFlags_WidthFixed, -1);
		for (int32_t _col_idx = 1; _col_idx < _column_count; ++_col_idx)
		{
			auto str = ct::toStr("G%i", _col_idx - 1);
			ImGui::TableSetupColumn(str.c_str(), ImGuiTableColumnFlags_WidthFixed, -1);
		}
		ImGui::TableHeadersRow();

		auto visible_count = LogEngine::Instance()->GetVisibleCount();

		// var for move singla from group ptr to group idx
		SignalSeriePtr move_signal_ptr = nullptr;
		GraphGroupPtr move_to_group_ptr = nullptr;

		int32_t visible_idx = 0;
		for (auto& item_cat : LogEngine::Instance()->GetSignalSeries())
		{
			for (auto& item_name : item_cat.second)
			{
				auto datas_ptr = item_name.second;
				if (datas_ptr)
				{
					if (datas_ptr->show)
					{
						if (ProjectFile::Instance()->m_AutoColorize)
						{
							datas_ptr->color_u32 = ImGui::GetColorU32(ct::toImVec4(GetRainBow(visible_idx, visible_count)));
							datas_ptr->color_v4 = ImGui::ColorConvertU32ToFloat4(datas_ptr->color_u32);
						}

						ImGui::PushID(ImGui::IncPUSHID());
						{
							ImGui::TableNextRow();

							ImGui::TableSetColumnIndex(0);
							if (ImGui::ColorEdit3("##colors", &datas_ptr->color_v4.x, ImGuiColorEditFlags_NoInputs))
							{
								datas_ptr->color_u32 = ImGui::GetColorU32(datas_ptr->color_v4);
								ProjectFile::Instance()->m_AutoColorize = false;
								ProjectFile::Instance()->SetProjectChange();
							}

							ImGui::TableSetColumnIndex(1);
							if (ImGui::Selectable(datas_ptr->name.c_str(), false,
								ImGuiSelectableFlags_SpanAllColumns |
								ImGuiSelectableFlags_AllowItemOverlap))
							{
								datas_ptr->show = !datas_ptr->show;

								LogEngine::Instance()->ShowHideSignal(
									datas_ptr->category, datas_ptr->name, datas_ptr->show);

								if (ProjectFile::Instance()->m_CollapseLogSelection)
								{
									LogPane::Instance()->PrepareLog();
								}

								ProjectFile::Instance()->SetProjectChange();
							}

							int32_t _col_idx = 0;
							for (auto& group_ptr : m_GraphGroups)
							{
								ImGui::TableSetColumnIndex(2 + _col_idx);
								ImGui::PushID(ImGui::IncPUSHID());
								{
									if (ImGui::RadioButtonLabeled(ImGui::GetFrameHeight(), "x", datas_ptr->graph_groupd_ptr == group_ptr, false))
									{
										move_signal_ptr = datas_ptr;
										move_to_group_ptr = group_ptr;
									}
								}
								ImGui::PopID();

								++_col_idx;
							}
						}
						ImGui::PopID();

						++visible_idx;
					}
				}
			}
		}

		ImGui::EndTable();

		// apply the move
		if (move_signal_ptr && move_to_group_ptr)
		{
			MoveSerieFromGroupToGroup(move_signal_ptr, move_signal_ptr->graph_groupd_ptr, move_to_group_ptr);
			ProjectFile::Instance()->SetProjectChange();

			move_signal_ptr = nullptr;
			move_to_group_ptr = nullptr;
		}
	}
}

void GraphView::DrawMenuBar()
{
	if (ImGui::BeginMenu("Settings"))
	{
		ImGui::MenuItem("Synchronize Graphs", nullptr, &ProjectFile::Instance()->m_SyncGraphs);
	
		if (ImGui::BeginMenu("Axis Labels"))
		{
			if (ImGui::MenuItem(m_show_hide_x_axis ? "Show X Axis LabelsR##GraphPaneDrawPanes" : "Hide X Axis LabelsR##GraphPaneDrawPanes"))
			{
				m_show_hide_x_axis = !m_show_hide_x_axis;
				m_need_show_hide_x_axis = true;
			}

			if (ImGui::MenuItem(m_show_hide_y_axis ? "Show Y Axis LabelsR##GraphPaneDrawPanes" : "Hide Y Axis LabelsR##GraphPaneDrawPanes"))
			{
				m_show_hide_y_axis = !m_show_hide_y_axis;
				m_need_show_hide_y_axis = true;
			}

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Colors"))
		{
			if (ImGui::ContrastedButton("R##ResetBarColor"))
			{
				ProjectFile::Instance()->m_GraphColors.graphBarColor =
                        s_DefaultGraphColors.graphBarColor;
			}

			ImGui::SameLine();

			ImGui::ColorEdit4("Bars color##tBarColor",
				&ProjectFile::Instance()->m_GraphColors.graphBarColor.x, ImGuiColorEditFlags_NoInputs);

			if (ImGui::ContrastedButton("R##ResetHoveredTimeBarColor"))
			{
				ProjectFile::Instance()->m_GraphColors.graphHoveredTimeColor =
                        s_DefaultGraphColors.graphHoveredTimeColor;
			}

			ImGui::SameLine();

			ImGui::ColorEdit4("Current Time color##HoveredTimeBarColor",
				&ProjectFile::Instance()->m_GraphColors.graphHoveredTimeColor.x, ImGuiColorEditFlags_NoInputs);

			if (ImGui::ContrastedButton("R##ResetMouseHoveredTimeBarColor"))
			{
				ProjectFile::Instance()->m_GraphColors.graphMouseHoveredTimeColor =
                        s_DefaultGraphColors.graphMouseHoveredTimeColor;
			}

			ImGui::SameLine();

			ImGui::ColorEdit4("Mouse Over color##MouseHoveredTimeBarColor",
				&ProjectFile::Instance()->m_GraphColors.graphMouseHoveredTimeColor.x, ImGuiColorEditFlags_NoInputs);

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

		ImGui::EndMenu();
	}

	ImGui::Text("Diff (?)");
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Diff :\npress key 'f' for first tick\npress key 's' for second tick\npress key 'r' for reset diff marks");
	}	
}

GraphGroupPtr GraphView::prGetGroupAt(const size_t& vIdx)
{
	if (m_GraphGroups.size() > vIdx)
	{
		auto it = std::next(m_GraphGroups.begin(), vIdx);
		if (it != m_GraphGroups.end())
		{
			return *it;
		}
	}

	return nullptr;
}

void GraphView::prEraseGroupAt(const size_t& vIdx)
{
	if (m_GraphGroups.size() > vIdx)
	{
		auto it = std::next(m_GraphGroups.begin(), vIdx);
		if (it != m_GraphGroups.end())
		{
			m_GraphGroups.erase(it);
		}
	}
}

bool GraphView::prBeginPlot(const std::string& vLabel, ct::dvec2 vRangeValue, const ImVec2& vSize, const bool& vFirstGraph) const
{
	const auto& time_range = LogEngine::Instance()->GetTicksTimeSerieRange();
	if (ImPlot::BeginPlot(vLabel.c_str(), vSize, ImPlotFlags_NoChild | ImPlotFlags_NoTitle))
	{
		if (m_need_show_hide_x_axis)
		{
			ImPlotPlot& plot = *GImPlot->CurrentPlot;
			ImPlotAxis& axis = plot.Axes[ImAxis_X1];
			ImFlipFlag(axis.Flags, ImPlotAxisFlags_NoTickLabels);
		}

		if (m_need_show_hide_y_axis)
		{
			ImPlotPlot& plot = *GImPlot->CurrentPlot;
			ImPlotAxis& axis = plot.Axes[ImAxis_Y1];
			ImFlipFlag(axis.Flags, ImPlotAxisFlags_NoTickLabels);
		}

		if (vFirstGraph)
		{
			ImPlot::SetupAxes(nullptr, nullptr, ImPlotAxisFlags_Opposite, ImPlotAxisFlags_Lock);
		}
		else
		{
			ImPlot::SetupAxes(nullptr, nullptr, ImPlotAxisFlags_NoTickLabels, ImPlotAxisFlags_Lock);
		}

		double y_offset = (vRangeValue.y - vRangeValue.x) * 0.1;
		if (IS_DOUBLE_EQUAL(y_offset, 0.0))
		{
			y_offset = 0.5;
		}

		ImPlot::SetupAxisLimits(ImAxis_X1, time_range.x, time_range.y);
		ImPlot::SetupAxisLimits(ImAxis_Y1, vRangeValue.x - y_offset, vRangeValue.y + y_offset, ImPlotCond_Always);

		ImPlot::SetupAxisScale(ImAxis_X1, ImPlotScale_Time);

		ImPlot::SetupAxisLimitsConstraints(ImAxis_X1, time_range.x, time_range.y);
		ImPlot::SetupAxisLimitsConstraints(ImAxis_Y1, vRangeValue.x - y_offset, vRangeValue.y + y_offset);
		ImPlot::SetupAxisFormat(ImAxis_Y1, "%.3f");

		if (ProjectFile::Instance()->m_SyncGraphs)
		{
			ImPlot::SetupAxisLinks(ImAxis_X1, &ProjectFile::Instance()->m_SyncGraphsLimits.X.Min, &ProjectFile::Instance()->m_SyncGraphsLimits.X.Max);
		}
		
		if (ImPlot::IsPlotHovered())
		{
			LogEngine::Instance()->SetHoveredTime(ImPlot::GetPlotMousePos().x);

			// first mark
			if (ImGui::IsKeyPressed(ImGuiKey_F))
			{
				LogEngine::Instance()->SetFirstDiffMark(ImPlot::GetPlotMousePos().x);
			}

			// second mark
			if (ImGui::IsKeyPressed(ImGuiKey_S))
			{
				LogEngine::Instance()->SetSecondDiffMark(ImPlot::GetPlotMousePos().x);
			}

			// second mark
			if (ImGui::IsKeyPressed(ImGuiKey_R))
			{
				LogEngine::Instance()->SetFirstDiffMark(0.0);
				LogEngine::Instance()->SetSecondDiffMark(0.0);
			}
		}

		return true;
	}

	return false;
}

void GraphView::prEndPlot()
{
	// draw diff first marks
	auto first_mark = ProjectFile::Instance()->m_DiffFirstMark;
	if (first_mark > 0.0)
	{
		if (ImPlot::DragLineX(DRAG_LINE_FIRST_DIFF_MARK, &ProjectFile::Instance()->m_DiffFirstMark, ProjectFile::Instance()->m_GraphColors.graphFirstDiffMarkColor, 1.5f))
		{
			LogEngine::Instance()->SetFirstDiffMark(ProjectFile::Instance()->m_DiffFirstMark);
		}

		ImPlot::TagX(ProjectFile::Instance()->m_DiffFirstMark, ProjectFile::Instance()->m_GraphColors.graphFirstDiffMarkColor, "%s", "|>");
	}

	// draw diff second marks
	auto second_mark = ProjectFile::Instance()->m_DiffSecondMark;
	if (second_mark > 0.0)
	{
		if (ImPlot::DragLineX(DRAG_LINE_SECOND_DIFF_MARK, &ProjectFile::Instance()->m_DiffSecondMark, ProjectFile::Instance()->m_GraphColors.graphSecondDiffMarkColor, 1.5f))
		{
			LogEngine::Instance()->SetSecondDiffMark(ProjectFile::Instance()->m_DiffSecondMark);
		}

		ImPlot::TagX(ProjectFile::Instance()->m_DiffSecondMark, ProjectFile::Instance()->m_GraphColors.graphSecondDiffMarkColor, "%s", "<|");
	}

	auto hovered_time = LogEngine::Instance()->GetHoveredTime();
	ImPlot::DragLineX(
		DRAG_LINE_LOG_HOVERED_TIME, &hovered_time, ProjectFile::Instance()->m_GraphColors.graphMouseHoveredTimeColor,
		1.5f, ImPlotDragToolFlags_NoInputs | ImPlotDragToolFlags_NoCursors);

	ImPlot::EndPlot();
}

void GraphView::prDrawSignalGraph_ImPlot(const SignalSerieWeak& vSignalSerie, const ImVec2& vSize, const bool& vFirstGraph)
{
	auto datas_ptr = vSignalSerie.lock();
	if (datas_ptr)
	{
		ImVec2 last_value_pos, value_pos;

		ImDrawList* draw_list = ImPlot::GetPlotDrawList();

		ImGui::PushID(ImGui::IncPUSHID());

		double hovered_time = LogEngine::Instance()->GetHoveredTime();
		const auto& name_str = datas_ptr->category + " / " + datas_ptr->name;
		if (prBeginPlot(name_str, datas_ptr->range_value, vSize, vFirstGraph))
		{
			if (ImPlot::BeginItem(name_str.c_str()))
			{
				ImPlot::GetCurrentItem()->Color = datas_ptr->color_u32;

				// render data
				auto& _data_values = datas_ptr->datas_values;
				if (!_data_values.empty())
				{
					//float zero_y = (float)ImPlot::PlotToPixels(0.0, 0.0).y;
					auto _data_ptr_0 = _data_values.at(0U).lock();
					if (_data_ptr_0)
					{
						double last_time = _data_ptr_0->time_epoch, current_time;
						double last_value = _data_ptr_0->value, current_value;
						last_value_pos = ImPlot::PlotToPixels(last_time, _data_ptr_0->value);

						for (size_t i = 1U; i < _data_values.size(); ++i)
						{
							auto _data_ptr_i = _data_values.at(i).lock();
							if (_data_ptr_i)
							{
								current_time = _data_ptr_i->time_epoch;
								current_value = _data_ptr_i->value;

								value_pos = ImPlot::PlotToPixels(current_time, current_value);
								ImPlot::FitPoint(ImPlotPoint(current_time, current_value));
								draw_list->AddLine(last_value_pos, ImVec2(value_pos.x, last_value_pos.y), datas_ptr->color_u32, 2.0f);
								draw_list->AddLine(ImVec2(value_pos.x, last_value_pos.y), value_pos, datas_ptr->color_u32, 2.0f);

								if (ImPlot::IsPlotHovered() &&
									hovered_time >= last_time &&
									hovered_time <= current_time)
								{
									auto pos = ImPlot::PlotToPixels(hovered_time, last_value);
									draw_list->AddLine(pos - ImVec2(20.0f, 0.0f), pos + ImVec2(20.0f, 0.0f),
										ImGui::GetColorU32(ProjectFile::Instance()->m_GraphColors.graphMouseHoveredTimeColor), 1.0f);
									draw_list->AddCircle(pos, 5.0f, ImGui::GetColorU32(ProjectFile::Instance()->m_GraphColors.graphHoveredTimeColor), 24, 2.0f);

									ImGui::BeginTooltipEx(ImGuiTooltipFlags_None, ImGuiWindowFlags_None);
									ImGui::Text("%s : %f", name_str.c_str(), last_value);
									ImGui::EndTooltip();
								}

								last_value_pos = value_pos;
								last_time = current_time;
								last_value = current_value;
							}
						}
					}
				}

				ImPlot::EndItem();
			}

			if (ImPlot::IsPlotHovered())
			{
				auto date_str = LogEngine::sConvertEpochToDateTimeString(hovered_time);
				ImGui::BeginTooltipEx(ImGuiTooltipFlags_None, ImGuiWindowFlags_None);
				ImGui::Text("time : %f\ndate : %s",
					hovered_time, date_str.c_str());
				ImGui::EndTooltip();
			}

			prEndPlot();
		}

		ImGui::PopID();
	}
}

void GraphView::DrawAloneGraphs(const GraphGroupPtr& vGraphGroupPtr, const ImVec2& vSize, bool& vFirstGraph)
{
	if (vGraphGroupPtr)
	{
		for (auto& cat : vGraphGroupPtr->GetSignalSeries())
		{
			for (auto& name : cat.second)
			{
				auto datas_ptr = name.second.lock();
				if (datas_ptr)
				{
					prDrawSignalGraph_ImPlot(datas_ptr, vSize, vFirstGraph);

					vFirstGraph = false;
				}
			}
		}
	}
}

void GraphView::DrawGroupedGraphs(const GraphGroupPtr& vGraphGroupPtr, const ImVec2& vSize, bool& vFirstGraph)
{
	if (vGraphGroupPtr)
	{
		if (!vGraphGroupPtr->GetSignalSeries().empty()) // if not empty
		{
			ImVec2 last_value_pos, value_pos;

			ImGui::PushID(ImGui::IncPUSHID());

			if (prBeginPlot(vGraphGroupPtr->GetName(), vGraphGroupPtr->GetSignalSeriesRange(), vSize, vFirstGraph))
			{
				const auto& hoveredTime = LogEngine::Instance()->GetHoveredTime();
				
				ImDrawList* draw_list = ImPlot::GetPlotDrawList();

				for (auto& cat : vGraphGroupPtr->GetSignalSeries())
				{
					for (auto& name : cat.second)
					{
						auto datas_ptr = name.second.lock();
						if (datas_ptr)
						{

							const auto& name_str = datas_ptr->category + " / " + datas_ptr->name; 
							if (ImPlot::BeginItem(name_str.c_str()))
							{
								ImPlot::GetCurrentItem()->Color = datas_ptr->color_u32;

								auto& _data_values = datas_ptr->datas_values;
								if (!_data_values.empty())
								{
									auto _data_ptr_0 = _data_values.at(0U).lock();
									if (_data_ptr_0)
									{
										double last_time = _data_ptr_0->time_epoch, current_time;
										double last_value = _data_ptr_0->value, current_value;
										last_value_pos = ImPlot::PlotToPixels(last_time, _data_ptr_0->value);
										for (size_t i = 1U; i < _data_values.size(); ++i)
										{
											auto _data_ptr_i = _data_values.at(i).lock();
											if (_data_ptr_i)
											{
												current_time = _data_ptr_i->time_epoch;
												current_value = _data_ptr_i->value;
												value_pos = ImPlot::PlotToPixels(current_time, current_value);

												ImPlot::FitPoint(ImPlotPoint(current_time, current_value));

												draw_list->AddLine(last_value_pos, ImVec2(value_pos.x, last_value_pos.y), datas_ptr->color_u32, 2.0f);
												draw_list->AddLine(ImVec2(value_pos.x, last_value_pos.y), value_pos, datas_ptr->color_u32, 2.0f);

												// draw gizmo for mouse over tick
												if (ImPlot::IsPlotHovered() &&
													hoveredTime >= last_time &&
													hoveredTime <= current_time)
												{
													auto pos = ImPlot::PlotToPixels(hoveredTime, last_value);
													draw_list->AddLine(pos - ImVec2(20.0f, 0.0f), pos + ImVec2(20.0f, 0.0f),
														ImGui::GetColorU32(ProjectFile::Instance()->m_GraphColors.graphMouseHoveredTimeColor), 1.0f);
													draw_list->AddCircle(pos, 5.0f, ImGui::GetColorU32(ProjectFile::Instance()->m_GraphColors.graphHoveredTimeColor), 24, 2.0f);

													ImGui::BeginTooltipEx(ImGuiTooltipFlags_None, ImGuiWindowFlags_None);
													ImGui::Text("%s : %f", datas_ptr->name.c_str(), last_value);
													ImGui::EndTooltip();
												}

												last_value_pos = value_pos;
												last_time = current_time;
												last_value = current_value;
											}
										}
									}
								}

								ImPlot::EndItem();
							}
						}
					}
				}

				if (ImPlot::IsPlotHovered())
				{
					ImPlotPoint plotHoveredMouse = ImPlot::GetPlotMousePos();
					auto date_str = LogEngine::sConvertEpochToDateTimeString(plotHoveredMouse.x);
					ImGui::BeginTooltipEx(ImGuiTooltipFlags_None, ImGuiWindowFlags_None);
					ImGui::Text("time : %f\ndate : %s", plotHoveredMouse.x, date_str.c_str());
					ImGui::EndTooltip();
				}

				prEndPlot();
			}

			ImGui::PopID();

			vFirstGraph = false;
		}
	}
}

void GraphView::ComputeGraphsCount()
{
	m_GraphsCount = 0;
	for (auto ggit = m_GraphGroups.begin(); ggit != m_GraphGroups.end(); ++ggit)
	{
		auto ptr = *ggit;
		if (ptr)
		{
			if (ggit == m_GraphGroups.begin())
			{
				for (auto& item_cat : ptr->GetSignalSeries())
				{
					for (auto& item_name : item_cat.second)
					{
						auto datas_ptr = item_name.second.lock();
						if (datas_ptr)
						{
							++m_GraphsCount;
						}
					}
				}
			}
			else if (!ptr->GetSignalSeries().empty())
			{
				++m_GraphsCount;
			}
		}
	}
}

int32_t GraphView::GetGraphCount() const
{
	return m_GraphsCount;
}
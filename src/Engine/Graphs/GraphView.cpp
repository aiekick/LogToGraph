#include "GraphView.h"

#include <implot/implot.h>
#include <implot/implot_internal.h>

#include <Contrib/ImWidgets/ImWidgets.h>

#include <imgui/imgui_internal.h>

#include <Contrib/ImWidgets/ImWidgets.h>

#include <Engine/Log/LogEngine.h>
#include <Engine/Log/SignalSerie.h>
#include <Engine/Log/SignalTick.h>

#include <Project/ProjectFile.h>

#include <Engine/Graphs/GraphGroup.h>

#include <ctools/Logger.h>

#include <Panes/LogPane.h>

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
	m_GraphGroups.push_back(GraphGroup::Create()); // default group
	m_GraphGroups.push_back(GraphGroup::Create()); // and G0 group
	m_Range_Value = SignalValueRange(0.5, -0.5) * DBL_MAX;
}

void GraphView::AddSerieToGroup(SignalSerieWeak vSignalSerie, const size_t& vToGroupIdx)
{
	auto ptr = vSignalSerie.lock();
	if (ptr)
	{
		while (m_GraphGroups.size() <= vToGroupIdx + 1) // +1 because always one more than the current max
		{
			m_GraphGroups.push_back(GraphGroup::Create());
		}
		
		auto group_ptr = prGetGroupAt(vToGroupIdx);
		if (group_ptr)
		{
			ptr->graph_groupd_idx = vToGroupIdx;

			group_ptr->AddSignalSerie(vSignalSerie);

			m_Range_Value.x = ct::mini(m_Range_Value.x, group_ptr->GetSignalSeriesRange().x);
			m_Range_Value.y = ct::maxi(m_Range_Value.y, group_ptr->GetSignalSeriesRange().y);
		}
	}
}

void GraphView::RemoveSerieFromGroup(SignalSerieWeak vSignalSerie, const size_t& vFromGroupIdx)
{
	auto ptr = vSignalSerie.lock();
	if (ptr)
	{
		if (m_GraphGroups.size() > vFromGroupIdx)
		{
			ptr->graph_groupd_idx = 0U; 
			auto group_ptr = prGetGroupAt(vFromGroupIdx);
			if (group_ptr)
			{
				group_ptr->RemoveSignalSerie(vSignalSerie);

				// tofix : dont erase if not offset of 2 or more
				// erase if this is the before last group and if he is empty()
				if (m_GraphGroups.size() == (vFromGroupIdx + 2U) &&
					group_ptr->GetSignalSeries().empty())
				{
					prEraseGroupAt(vFromGroupIdx);
				}
			}
		}
	}
}

void GraphView::MoveSerieFromGroupToGroup(SignalSerieWeak vSignalSerie, const size_t& vFromGroupIdx, const size_t& vToGroupIdx)
{
	RemoveSerieFromGroup(vSignalSerie, vFromGroupIdx);
	AddSerieToGroup(vSignalSerie, vToGroupIdx);
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

	const int32_t _column_count = (int32_t)m_GraphGroups.size();
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

							for (int32_t _col_idx = 0; _col_idx < _column_count; ++_col_idx)
							{
								ImGui::TableSetColumnIndex(2 + _col_idx);
								ImGui::PushID(ImGui::IncPUSHID());
								{
									if (ImGui::RadioButtonLabeled(ImGui::GetFrameHeight(), "x", datas_ptr->graph_groupd_idx == _col_idx, false))
									{
										MoveSerieFromGroupToGroup(datas_ptr, datas_ptr->graph_groupd_idx, _col_idx);
										ProjectFile::Instance()->SetProjectChange();
									}
								}
								ImGui::PopID();
							}
						}
						ImGui::PopID();

						++visible_idx;
					}
				}
			}
		}

		ImGui::EndTable();
	}
}

void GraphView::DrawGraphs()
{
	//prDrawGraph_ImPlot();
	prDrawGraph_NewSystem();
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

void GraphView::prDrawSignalGraph_ImPlot(SignalSerieWeak vSignalSerie)
{
	auto datas_ptr = vSignalSerie.lock();
	if (datas_ptr)
	{
		auto time_range = LogEngine::Instance()->GetTicksTimeSerieRange();
		
		const ImU32 color_yellow = ImGui::GetColorU32(ProjectFile::Instance()->m_GraphHoveredTimeColor);
		const ImU32 color_green = ImGui::GetColorU32(ProjectFile::Instance()->m_GraphMouseHoveredTimeColor);
		ImVec2 last_value_pos, value_pos, hovered_min_pos, hovered_max_pos;

		const auto& name_str = datas_ptr->category + " / " + datas_ptr->name;
		if (ImPlot::BeginPlot(name_str.c_str(), ImVec2(-1, 200), ImPlotFlags_NoLegend))
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

			ImPlot::SetupAxes(NULL, NULL, ImPlotAxisFlags_NoTickLabels, ImPlotAxisFlags_AutoFit | ImPlotAxisFlags_RangeFit);

			double y_offset = (datas_ptr->range_value.y - datas_ptr->range_value.x) * 0.1;
			ImPlot::SetupAxesLimits(time_range.x, time_range.y, datas_ptr->range_value.x - y_offset, datas_ptr->range_value.y + y_offset);

			ImPlot::SetupAxisScale(ImAxis_X1, ImPlotScale_Time);

			ImPlot::SetupAxisLimitsConstraints(ImAxis_X1, time_range.x, time_range.y);
			ImPlot::SetupAxisFormat(ImAxis_Y1, "%.2f");

			ImDrawList* draw_list = ImPlot::GetPlotDrawList();

			bool plotHovered = ImPlot::IsPlotHovered();
			ImPlotPoint plotHoveredMouse = ImPlot::GetPlotMousePos();
			//plotHoveredMouse.x = ImPlot::RoundTime(ImPlotTime::FromDouble(plotHoveredMouse.x), ImPlotTimeUnit_Day).ToDouble();

			if (ImPlot::BeginItem(datas_ptr->name.c_str()))
			{
				ImPlot::GetCurrentItem()->Color = datas_ptr->color_u32;

				double hovered_time = LogEngine::Instance()->GetHoveredTime();

				// render data
				if (datas_ptr->datas_values.size() > 0U)
				{
					float zero_y = (float)ImPlot::PlotToPixels(0.0, 0.0).y;
					auto _data_ptr_0 = datas_ptr->datas_values.at(0U).lock();
					if (_data_ptr_0)
					{
						double last_time = _data_ptr_0->time_epoch, current_time;
						double last_value = _data_ptr_0->value, current_value;
						last_value_pos = ImPlot::PlotToPixels(last_time, _data_ptr_0->value);

						for (size_t i = 1U; i < datas_ptr->datas_values.size(); ++i)
						{
							auto _data_ptr_i = datas_ptr->datas_values.at(i).lock();
							if (_data_ptr_i)
							{
								current_time = _data_ptr_i->time_epoch;
								current_value = _data_ptr_i->value;
								value_pos = ImPlot::PlotToPixels(current_time, current_value);

								draw_list->AddLine(last_value_pos, ImVec2(value_pos.x, last_value_pos.y), datas_ptr->color_u32, 2.0f);
								draw_list->AddLine(ImVec2(value_pos.x, last_value_pos.y), value_pos, datas_ptr->color_u32, 2.0f);
								//draw_list->AddRectFilled(ImVec2(last_value_pos.x, zero_y), ImVec2(value_pos.x, last_value_pos.y), datas.color);

								if (hovered_time >= last_time &&
									hovered_time <= current_time)
								{
									hovered_min_pos = ImPlot::PlotToPixels(hovered_time, datas_ptr->range_value.x);
									hovered_max_pos = ImPlot::PlotToPixels(hovered_time, datas_ptr->range_value.y);
									draw_list->AddLine(hovered_min_pos, hovered_max_pos, color_yellow, 4.0f);
								}

								if (plotHovered &&
									plotHoveredMouse.x >= last_time &&
									plotHoveredMouse.x <= current_time)
								{
									hovered_min_pos = ImPlot::PlotToPixels(plotHoveredMouse.x, datas_ptr->range_value.x);
									hovered_max_pos = ImPlot::PlotToPixels(plotHoveredMouse.x, datas_ptr->range_value.y);
									draw_list->AddLine(hovered_min_pos, hovered_max_pos, color_green, 2.0f);
									auto pos = ImVec2(hovered_min_pos.x, last_value_pos.y);
									draw_list->AddLine(pos - ImVec2(20.0f, 0.0f), pos + ImVec2(20.0f, 0.0f), color_green, 2.0f);
									draw_list->AddCircle(pos, 5.0f, color_yellow, 24, 2.0f);

									// 1668687822.067365000 => 17/11/2022 13:23:42.067365000
									double seconds = ct::fract(plotHoveredMouse.x); // 0.067365000
									std::time_t _epoch_time = (std::time_t)plotHoveredMouse.x;
									auto tm = std::localtime(&_epoch_time);
									double _sec = (double)tm->tm_sec + seconds;
									auto date_str = ct::toStr("%i/%i/%i %i:%i:%f",
										tm->tm_year + 1900, tm->tm_mon, tm->tm_mday,
										tm->tm_hour, tm->tm_min, _sec);
									ImGui::SetTooltip("%s : %f\ntime : %f\ndate : %s", datas_ptr->name.c_str(), last_value, plotHoveredMouse.x, date_str.c_str());
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

			ImPlot::EndPlot();
		}
	}
}

void GraphView::prDrawGraph_ImPlot()
{
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

			if (ImGui::ContrastedButton("R##ResetMouseHoveredTimeBarColor"))
			{
				ProjectFile::Instance()->m_GraphMouseHoveredTimeColor =
					ProjectFile::Instance()->m_DefaultGraphMouseHoveredTimeColor;
			}

			ImGui::SameLine();

			ImGui::ColorEdit4("Mouse Over Color##MouseHoveredTimeBarColor",
				&ProjectFile::Instance()->m_GraphMouseHoveredTimeColor.x, ImGuiColorEditFlags_NoInputs);

			ImGui::EndMenu();
		}

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

		ImGui::EndMenuBar();
	}

	ImPlot::GetStyle().UseLocalTime = false;
	ImPlot::GetStyle().UseISO8601 = false;
	ImPlot::GetStyle().Use24HourClock = false;

	auto time_range = LogEngine::Instance()->GetTicksTimeSerieRange();
	//auto time_serie = LogEngine::Instance()->GetSignalTicks();

	//const ImU32 color_bars = ImGui::GetColorU32(ProjectFile::Instance()->m_GraphBarColor);
	const ImU32 color_yellow = ImGui::GetColorU32(ProjectFile::Instance()->m_GraphHoveredTimeColor);
	const ImU32 color_green = ImGui::GetColorU32(ProjectFile::Instance()->m_GraphMouseHoveredTimeColor);
	ImVec2 last_value_pos, value_pos, hovered_min_pos, hovered_max_pos;

	auto visible_count = LogEngine::Instance()->GetVisibleCount();

	int32_t visible_idx = 0;
	for (auto& item_cat : LogEngine::Instance()->GetSignalSeries())
	{
		for (auto& item_name : item_cat.second)
		{
			auto datas_ptr = item_name.second;
			if (datas_ptr)
			{
				//assert(time_serie.size() == datas_ptr->datas_values.size());

				if (datas_ptr->show)
				{
					//datas_ptr->color_u32 = ImGui::GetColorU32(ct::toImVec4(GetRainBow(visible_idx, visible_count)));
					//datas_ptr->color_v4 = ImGui::ColorConvertU32ToFloat4(datas_ptr->color_u32);

					prDrawSignalGraph_ImPlot(datas_ptr);

					++visible_idx;
				}
			}
		}
	}
}

void GraphView::prDrawGraph_NewSystem()
{
	size_t idx = 0U;
	for (auto graph_group_ptr : m_GraphGroups)
	{
		if (graph_group_ptr)
		{
			if (idx == 0U)
			{
				// idx == 0 => signal graphs are independant
				for (auto& cat : graph_group_ptr->GetSignalSeries())
				{
					for (auto& name : cat.second)
					{
						auto datas_ptr = name.second.lock();
						if (datas_ptr)
						{
							prDrawSignalGraph_ImPlot(datas_ptr);
						}
					}
				}
			}
			else if (!graph_group_ptr->GetSignalSeries().empty()) // if not empty
			{
				const auto& time_range = LogEngine::Instance()->GetTicksTimeSerieRange();
				const auto& range_value = graph_group_ptr->GetSignalSeriesRange();

				const ImU32 color_yellow = ImGui::GetColorU32(ProjectFile::Instance()->m_GraphHoveredTimeColor);
				const ImU32 color_green = ImGui::GetColorU32(ProjectFile::Instance()->m_GraphMouseHoveredTimeColor);
				ImVec2 last_value_pos, value_pos, hovered_min_pos, hovered_max_pos;

				ImGui::PushID(ImGui::IncPUSHID());
				{
					if (ImPlot::BeginPlot(ct::toStr("G%u", (uint32_t)idx - 1U).c_str(), ImVec2(-1, 200), ImPlotFlags_NoLegend))
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

						ImPlot::SetupAxes(NULL, NULL, ImPlotAxisFlags_NoTickLabels, ImPlotAxisFlags_AutoFit | ImPlotAxisFlags_RangeFit);

						double y_offset = (range_value.y - range_value.x) * 0.1;
						ImPlot::SetupAxesLimits(time_range.x, time_range.y, range_value.x - y_offset, range_value.y + y_offset);

						ImPlot::SetupAxisScale(ImAxis_X1, ImPlotScale_Time);

						ImPlot::SetupAxisLimitsConstraints(ImAxis_X1, time_range.x, time_range.y);
						ImPlot::SetupAxisFormat(ImAxis_Y1, "%.2f");

						ImDrawList* draw_list = ImPlot::GetPlotDrawList();

						bool plotHovered = ImPlot::IsPlotHovered();
						ImPlotPoint plotHoveredMouse = ImPlot::GetPlotMousePos();
						std::string plotHoveredMouse_tooltip_values;
						//plotHoveredMouse.x = ImPlot::RoundTime(ImPlotTime::FromDouble(plotHoveredMouse.x), ImPlotTimeUnit_Day).ToDouble();

						// idx != 0 -> signal graphs are grouped
						for (auto& cat : graph_group_ptr->GetSignalSeries())
						{
							for (auto& name : cat.second)
							{
								auto datas_ptr = name.second.lock();
								if (datas_ptr)
								{
									if (ImPlot::BeginItem(datas_ptr->name.c_str()))
									{
										ImPlot::GetCurrentItem()->Color = datas_ptr->color_u32;

										double hovered_time = LogEngine::Instance()->GetHoveredTime();

										// render data
										if (datas_ptr->datas_values.size() > 0U)
										{
											float zero_y = (float)ImPlot::PlotToPixels(0.0, 0.0).y;
											auto _data_ptr_0 = datas_ptr->datas_values.at(0U).lock();
											if (_data_ptr_0)
											{
												double last_time = _data_ptr_0->time_epoch, current_time;
												double last_value = _data_ptr_0->value, current_value;
												last_value_pos = ImPlot::PlotToPixels(last_time, _data_ptr_0->value);
												for (size_t i = 1U; i < datas_ptr->datas_values.size(); ++i)
												{
													auto _data_ptr_i = datas_ptr->datas_values.at(i).lock();
													if (_data_ptr_i)
													{
														current_time = _data_ptr_i->time_epoch;
														current_value = _data_ptr_i->value;
														value_pos = ImPlot::PlotToPixels(current_time, current_value);

														draw_list->AddLine(last_value_pos, ImVec2(value_pos.x, last_value_pos.y), datas_ptr->color_u32, 2.0f);
														draw_list->AddLine(ImVec2(value_pos.x, last_value_pos.y), value_pos, datas_ptr->color_u32, 2.0f);
														//draw_list->AddRectFilled(ImVec2(last_value_pos.x, zero_y), ImVec2(value_pos.x, last_value_pos.y), datas.color);

														if (hovered_time >= last_time &&
															hovered_time <= current_time)
														{
															hovered_min_pos = ImPlot::PlotToPixels(hovered_time, datas_ptr->range_value.x);
															hovered_max_pos = ImPlot::PlotToPixels(hovered_time, datas_ptr->range_value.y);
															draw_list->AddLine(hovered_min_pos, hovered_max_pos, color_yellow, 4.0f);
														}

														// draw gizmo for mouse over tick
														if (plotHovered &&
															plotHoveredMouse.x >= last_time &&
															plotHoveredMouse.x <= current_time)
														{
															hovered_min_pos = ImPlot::PlotToPixels(plotHoveredMouse.x, datas_ptr->range_value.x);
															hovered_max_pos = ImPlot::PlotToPixels(plotHoveredMouse.x, datas_ptr->range_value.y);
															draw_list->AddLine(hovered_min_pos, hovered_max_pos, color_green, 2.0f);
															auto pos = ImVec2(hovered_min_pos.x, last_value_pos.y);
															draw_list->AddLine(pos - ImVec2(20.0f, 0.0f), pos + ImVec2(20.0f, 0.0f), color_green, 2.0f);
															draw_list->AddCircle(pos, 5.0f, color_yellow, 24, 2.0f);

															ImGui::BeginTooltipEx(ImGuiTooltipFlags_None, ImGuiWindowFlags_None);
															ImGui::PushStyleColor(ImGuiCol_Text, datas_ptr->color_u32);
															ImGui::Text("%s : %f", datas_ptr->name.c_str(), last_value);
															ImGui::PopStyleColor();
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

						// draw tooltip
						if (plotHovered)
						{
							// 1668687822.067365000 => 17/11/2022 13:23:42.067365000
							double seconds = ct::fract(plotHoveredMouse.x); // 0.067365000
							std::time_t _epoch_time = (std::time_t)plotHoveredMouse.x;
							auto tm = std::localtime(&_epoch_time);
							double _sec = (double)tm->tm_sec + seconds;
							auto date_str = ct::toStr("%i/%i/%i %i:%i:%f",
								tm->tm_year + 1900, tm->tm_mon, tm->tm_mday,
								tm->tm_hour, tm->tm_min, _sec);

							ImGui::BeginTooltipEx(ImGuiTooltipFlags_None, ImGuiWindowFlags_None);
							ImGui::Text("time : %f\ndate : %s", plotHoveredMouse.x, date_str.c_str());
							ImGui::EndTooltip();
						}

						ImPlot::EndPlot();
					}
				}
				ImGui::PopID();
			}
		}

		++idx;
	}
}

std::string GraphView::getXml(const std::string& vOffset, const std::string& /*vUserDatas*/)
{
	std::string str;

	return str;
}

bool GraphView::setFromXml(tinyxml2::XMLElement* vElem, tinyxml2::XMLElement* vParent, const std::string& /*vUserDatas*/)
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
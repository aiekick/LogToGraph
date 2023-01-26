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

#include "GraphAnnotation.h"

#include <imgui/imgui_internal.h>
#include <Engine/Log/SignalSerie.h>
#include <Panes/Manager/LayoutManager.h>
#include <Project/ProjectFile.h>
#include <chrono>

//////////////////////////////////////////////////////
///// STATIC /////////////////////////////////////////
//////////////////////////////////////////////////////

GraphAnnotationPtr GraphAnnotation::Create()
{
	auto res = std::make_shared<GraphAnnotation>();
	res->m_This = res;
	return res;
}


bool GraphAnnotation::IsMouseHoverLine(const ct::dvec2& vMousePos, const double& vRadius, const ct::dvec2& vStart, const ct::dvec2& vEnd)
{
	// line sdf // https://iquilezles.org/articles/distfunctions2d/
	// d < 0.0 => inside
	// d > 0.0 => outside

	const auto a = vMousePos - vEnd;
	const auto b = vStart - vEnd;
	auto h = ct::clamp(ct::dot(a, b) / ct::dot(b, b), 0., 1.);
	const auto pp = a - b * h;
	const auto dist_to_line = ct::dot(pp, pp) - vRadius * vRadius;
	return (dist_to_line < 0.0);
}

//////////////////////////////////////////////////////
///// PUBLIC /////////////////////////////////////////
//////////////////////////////////////////////////////

void GraphAnnotation::Clear()
{
	m_ElapsedTimeStr.clear();
	m_StartPos = ImPlotPoint();
	m_EndPos = ImPlotPoint();
	m_LabelPos = ImPlotPoint();
	m_Label = nullptr;
	m_Color = ImVec4();
}

void GraphAnnotation::SetStartPoint(const ImPlotPoint& vStartPoint)
{
	// we save directly the point in plot coord, because the plot can be scaled
	// and we need for draw to put back in screen coord accroding to current graph scale
	m_StartPos = vStartPoint;
}

void GraphAnnotation::SetEndPoint(const ImPlotPoint& vEndPoint)
{
	// we save directly the point in plot coord, because the plot can be scaled
	// and we need for draw to put back in screen coord accroding to current graph scale
	m_EndPos = vEndPoint;

	ComputeElapsedTime();

	m_LabelPos.x = (m_StartPos.x + m_EndPos.x) * 0.5;
	m_LabelPos.y = (m_StartPos.y + m_EndPos.y) * 0.5;

	m_Color = ProjectFile::Instance()->m_GraphColors.graphHoveredTimeColor;
}

void GraphAnnotation::ComputeElapsedTime()
{
	int64_t nano_seconds = static_cast<int64_t>((m_EndPos.x - m_StartPos.x) * 1e9);
	int64_t micro_seconds = nano_seconds / 1000;
	int64_t milli_seconds = micro_seconds / 1000;
	int64_t seconds = milli_seconds / 1000;
	int64_t minutes = seconds / 60;
	int64_t hours = minutes / 60;
	int64_t days = hours / 24;

	nano_seconds = nano_seconds % 1000;
	micro_seconds = micro_seconds % 1000;
	milli_seconds = milli_seconds % 1000;
	seconds = seconds % 60;
	minutes = minutes % 60;
	hours = hours % 24;

	m_ElapsedTimeStr.clear();

	// elapsed time dont need year or month, the biggest supported unity is day count
	
	// todo : can be optimized in time i guess ...
	if (days) {
		m_ElapsedTimeStr += ct::toStr("%iD:", days);
	}
	if (hours) {
		m_ElapsedTimeStr += ct::toStr("%iH:", hours);
	}
	if (minutes) {
		m_ElapsedTimeStr += ct::toStr("%im:", minutes);
	}
	if (seconds) {
		m_ElapsedTimeStr += ct::toStr("%is:", seconds);
	}
	if (milli_seconds) {
		m_ElapsedTimeStr += ct::toStr("%ims:", milli_seconds);
	}
	if (micro_seconds) {
		m_ElapsedTimeStr += ct::toStr("%ius:", micro_seconds);
	}
	if (nano_seconds) {
		m_ElapsedTimeStr += ct::toStr("%ins:", nano_seconds);
	}

	// for ImPlot
	m_Label = m_ElapsedTimeStr.c_str();
}

void GraphAnnotation::DrawToPoint(const ImVec2& vMousePoint)
{
	auto win_ptr = ImGui::GetCurrentWindowRead();
	if (win_ptr) 
	{
		auto draw_list_ptr = win_ptr->DrawList;
		if (draw_list_ptr) 
		{
			const auto col = ImGui::GetColorU32(ProjectFile::Instance()->m_GraphColors.graphHoveredTimeColor);
			const auto st = ImPlot::PlotToPixels(m_StartPos);
			const auto en = vMousePoint;

			draw_list_ptr->AddLine(st, en, col, 2.0f);
			draw_list_ptr->AddCircleFilled(st, 5.0f, col, 24);
			draw_list_ptr->AddCircleFilled(en, 5.0f, col, 24);
		}
	}
}

void GraphAnnotation::Draw()
{
	if (m_Label)
	{
		ImGui::PushID(this);

		auto win_ptr = ImGui::GetCurrentWindowRead();
		if (win_ptr)
		{
			auto draw_list_ptr = win_ptr->DrawList;
			if (draw_list_ptr)
			{
				// todo : to optimize
				const auto col = ImGui::GetColorU32(ProjectFile::Instance()->m_GraphColors.graphHoveredTimeColor);
				const auto st = ImPlot::PlotToPixels(m_StartPos);
				const auto en = ImPlot::PlotToPixels(m_EndPos);

				draw_list_ptr->AddLine(st, en, col, 2.0f);
				draw_list_ptr->AddCircleFilled(st, 5.0f, col, 24);
				draw_list_ptr->AddCircleFilled(en, 5.0f, col, 24);

				ImPlot::Annotation(
					m_LabelPos.x, m_LabelPos.y,
					m_Color,
					ImVec2(-15, 15), true, "%s", m_Label);
			}
		}

		ImGui::PopID();
	}
}

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

#include <models/log/SignalSerie.h>
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

static inline double s_dot(ImPlotPoint a, ImPlotPoint b) { return a.x * b.x + a.y * b.y; }
static inline ImPlotPoint operator - (ImPlotPoint v, ImPlotPoint f) { return ImPlotPoint(v.x - f.x, v.y - f.y); }
static inline ImPlotPoint operator + (ImPlotPoint v, ImPlotPoint f) { return ImPlotPoint(v.x + f.x, v.y + f.y); }
static inline ImPlotPoint operator * (double v, ImPlotPoint f) { return ImPlotPoint(v * f.x, v * f.y); }
static inline ImPlotPoint operator * (ImPlotPoint f, double v) { return ImPlotPoint(v * f.x, v * f.y); }
static inline ImPlotPoint operator / (ImPlotPoint v, double f) { return ImPlotPoint(v.x * f, v.y / f); }
static inline ez::dvec2 s_toDVec2(const ImPlotPoint& v) { return ez::dvec2(v.x, v.y); }
static inline ImPlotPoint ImpClamp(ImPlotPoint v, double a, double b) { return ImPlotPoint(ez::clamp(v.x,a,b), ez::clamp(v.y, a, b)); }
static inline double ImpDot(ImPlotPoint a, ImPlotPoint b) { return a.x * b.x + a.y * b.y; }
static inline double ImpLength(ImPlotPoint a) { return sqrt(ImpDot(a,a)); }
static inline double ImpDistance(ImPlotPoint A, ImPlotPoint B) { double dx = A.x - B.x;	double dy = A.y - B.y; return sqrt(dx * dx + dy * dy); }

bool GraphAnnotation::sIsMouseHoverLine(const ez::dvec2& vMousePos, const double& vRadius, const ez::dvec2& vStart, const ez::dvec2& vEnd, ez::dvec2& vOutLinePoint)
{
	const auto mp = ImPlot::PixelsToPlot(vMousePos);
	const auto st = ImPlot::PixelsToPlot(vStart);
	const auto en = ImPlot::PixelsToPlot(vEnd);

	const auto a = mp - st;
	const auto b = en - st;
	const auto dot_b = s_dot(b, b);
	if (ez::isEqual(dot_b, 0.0))
		return false;

	//projected point on infinite line
	auto proj_pt = st + s_dot(a, b) * b / dot_b;

	// limitation of projected point to the extremities of the segments
	if (s_dot(proj_pt - st, en - st) <= 0.0)
	{
		vOutLinePoint = vStart;
	}
	else if (s_dot(proj_pt - en, st - en) <= 0.0)
	{
		vOutLinePoint = vEnd;
	}
	else
	{
		vOutLinePoint = s_toDVec2(ImPlot::PlotToPixels(proj_pt));
	}

	auto dist_to_line = (vMousePos - vOutLinePoint).length();
	return (dist_to_line <= vRadius);
}

bool GraphAnnotation::sIsMouseHoverLine2P(const ImVec2& vMousePos, const double& vRadius, const ImVec2& vStart, const ImVec2& vEnd, ImVec2& vOutLinePoint, double& vOutDistToLine)
{
	const auto P = ImPlot::PixelsToPlot(vMousePos);
	const auto A = ImPlot::PixelsToPlot(vStart);
	const auto B = ImPlot::PixelsToPlot(vEnd);
	
	const auto BA = B - A;
	const auto PA = P - A;

	const auto id = ImpDot(BA, BA);
	if (ez::isEqual(id, 0.0))
		return false;

	const auto H = ez::clamp(ImpDot(PA, PA) / id, 0.0, 1.0);
	const auto Q = PA - H * BA;
	const auto D = ImpLength(Q);

	vOutLinePoint = ImPlot::PlotToPixels(P - Q);
	vOutDistToLine = D;

	ImDrawList* draw_list = ImPlot::GetPlotDrawList();
	draw_list->AddLine(vStart, vEnd, ImGui::GetColorU32(ImVec4(1, 1, 0, 1)), 2.0);
	draw_list->AddLine(vMousePos, vStart, ImGui::GetColorU32(ImVec4(0, 1, 0, 1)), 2.0);
	draw_list->AddLine(vMousePos, vEnd, ImGui::GetColorU32(ImVec4(0, 0, 1, 1)), 2.0);
	draw_list->AddLine(vMousePos, vOutLinePoint, ImGui::GetColorU32(ImVec4(1,0,1,1)), 2.0);

	return (vOutDistToLine <= vRadius);
}

bool GraphAnnotation::sIsMouseHoverLine3P(const ImVec2& vMousePos, const double& vRadius, const ImVec2& vStart, const ImVec2& vMiddle, const ImVec2& vEnd, ImVec2& vOutLinePoint)
{
	ImVec2 p0, p1;
	double d0, d1;
	sIsMouseHoverLine2P(vMousePos, vRadius, vStart, vMiddle, p0, d0);
	sIsMouseHoverLine2P(vMousePos, vRadius, vMiddle, vEnd, p1, d1);

	if (d0 < d1)
	{
		vOutLinePoint = p0;
		return (d0 <= vRadius);

	}

	vOutLinePoint = p1;
	return (d1 <= vRadius);
}

bool GraphAnnotation::sIsMouseHoverLine4P(const ImVec2& vMousePos, const double& vRadius, const ImVec2& vp0, const ImVec2& vp1, const ImVec2& vp2, const ImVec2& vp3, ImVec2& vOutLinePoint)
{
	const auto M = ImPlot::PixelsToPlot(vMousePos);
	const auto A = ImPlot::PixelsToPlot(vp0);
	const auto B = ImPlot::PixelsToPlot(vp1);
	const auto C = ImPlot::PixelsToPlot(vp2);
	const auto D = ImPlot::PixelsToPlot(vp3);

	/*
	   B ---- C
	   |      |
	   |   P  D
	   |
	   A

	      ou

	   A
	   |      
	   |   P  D
	   |      |
	   B ---- C
	*/

	ImPlotPoint pH = { M.x, B.y };
	ImPlotPoint pV1 = { A.x, M.y };
	ImPlotPoint pV2 = { D.x, M.y };

	ImPlotPoint P;
	double dist = 0.0;

	if (M.x >= A.x && M.x <= D.x) {
		double distH = ImpDistance(M, pH);
		double distV1 = ImpDistance(M, pV1);
		double distV2 = ImpDistance(M, pV2);
		if (distH <= distV1 && distH <= distV2) {
			P = pH;
			dist = distH;
		}
		else if (distV1 <= distH && distV1 <= distV2) {
			P = pV1;
			dist = distH;
		}
		else {
			P = pV2;
			dist = distV2;
		}
	}
	else if (M.x < A.x) {
		P = pV1;
		dist = ImpDistance(M, pV2);
	}
	else {
		P = pV2;
		dist = ImpDistance(M, pV1);
	}

	vOutLinePoint = ImPlot::PlotToPixels(P);

	/*ImDrawList* draw_list = ImPlot::GetPlotDrawList();
	draw_list->AddLine(vMousePos, vOutLinePoint, ImGui::GetColorU32(ImVec4(1, 0, 1, 1)), 2.0);
	draw_list->AddCircleFilled(vp0, 10.0f, ImGui::GetColorU32(ImVec4(1, 0, 1, 1)));
	draw_list->AddCircleFilled(vp1, 10.0f, ImGui::GetColorU32(ImVec4(1, 0, 1, 1)));
	draw_list->AddCircleFilled(vp2, 10.0f, ImGui::GetColorU32(ImVec4(1, 0, 1, 1)));
	draw_list->AddCircleFilled(vp3, 10.0f, ImGui::GetColorU32(ImVec4(1, 0, 1, 1)));*/

	return (dist <= vRadius);
}

std::string GraphAnnotation::sGetHumanReadableElapsedTime(const double& vElapsedTime)
{
	// always positiv delta
	int64_t nano_seconds = static_cast<int64_t>(ez::abs((vElapsedTime) * 1e9));
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

	std::string res;

	// elapsed time dont need year or month, the biggest supported unity is day count

	// todo : can be optimized in time i guess ...
	if (days) {
		res += ez::str::toStr("%iD:", days);
	}
	if (hours) {
		res += ez::str::toStr("%iH:", hours);
	}
	if (minutes) {
		res += ez::str::toStr("%im:", minutes);
	}
	if (seconds) {
		res += ez::str::toStr("%is:", seconds);
	}
	if (milli_seconds) {
		res += ez::str::toStr("%ims:", milli_seconds);
	}
	if (micro_seconds) {
		res += ez::str::toStr("%ius:", micro_seconds);
	}
	if (nano_seconds) {
		res += ez::str::toStr("%ins:", nano_seconds);
	}

	return res;
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
	m_ImGuiLabel = nullptr;
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

void GraphAnnotation::SetSignalSerieParent(const SignalSerieWeak& vSignalSerie)
{
	m_ParentSignalSerie = vSignalSerie;
}

SignalSerieWeak GraphAnnotation::GetParentSignalSerie()
{
	return m_ParentSignalSerie;
}

ImGuiLabel GraphAnnotation::GetImGuiLabel() const
{
	return m_ImGuiLabel;
}

void GraphAnnotation::DrawToPoint(SignalSeriePtr vSignalSeriePtr, const ImVec2& vMousePoint)
{
	if (vSignalSeriePtr && vSignalSeriePtr == m_ParentSignalSerie.lock())
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
}

void GraphAnnotation::Draw()
{
	if (m_ImGuiLabel)
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
					ImVec2(-15, 15), true, "%s", m_ImGuiLabel);
			}
		}

		ImGui::PopID();
	}
}

//////////////////////////////////////////////////////
///// PRIVATE ////////////////////////////////////////
//////////////////////////////////////////////////////

void GraphAnnotation::ComputeElapsedTime()
{
	m_ElapsedTimeStr = GraphAnnotation::sGetHumanReadableElapsedTime(m_EndPos.x - m_StartPos.x);
	
	// for ImPlot
	m_ImGuiLabel = m_ElapsedTimeStr.c_str();
}

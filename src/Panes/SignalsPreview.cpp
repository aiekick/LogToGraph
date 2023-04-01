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

#include "SignalsPreview.h"
#include <Gui/MainFrame.h>
#include <ctools/cTools.h>
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

bool SignalsPreview::Init()
{
	return true;
}

void SignalsPreview::Unit()
{

}

int SignalsPreview::DrawPanes(const uint32_t& /*vCurrentFrame*/, const int& vWidgetId, const std::string& /*vvUserDatas*/, PaneFlag& vInOutPaneShown)
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

void SignalsPreview::DrawDialogsAndPopups(const uint32_t& /*vCurrentFrame*/, const std::string& /*vvUserDatas*/)
{

}

int SignalsPreview::DrawWidgets(const uint32_t& /*vCurrentFrame*/, const int& vWidgetId, const std::string& /*vvUserDatas*/)
{
	return vWidgetId;
}

void SignalsPreview::Clear()
{
	m_PreviewTicks.clear();
}

void SignalsPreview::SetHoveredTime(const SignalEpochTime& vHoveredTime)
{
	size_t count_signals = LogEngine::Instance()->GetSignalsCount();
	
	if (m_PreviewTicks.empty())
	{
		m_PreviewTicks.resize(count_signals);
	}

	size_t idx = 0U;
	size_t visible_idx = 0U;
	size_t visible_count = LogEngine::Instance()->GetVisibleCount();
	for (auto& item_cat : LogEngine::Instance()->GetSignalSeries())
	{
		for (auto& item_name : item_cat.second)
		{
			if (item_name.second)
			{
				SignalTickPtr last_ptr = nullptr;
				for (const auto& tick_weak : item_name.second->datas_values)
				{
					auto ptr = tick_weak.lock();
					if (last_ptr && vHoveredTime >= last_ptr->time_epoch && 
						ptr && vHoveredTime <= ptr->time_epoch)
					{
						if (idx < count_signals)
						{
							m_PreviewTicks[idx] = last_ptr;

							if (ProjectFile::Instance()->m_AutoColorize)
							{
								auto parent_ptr = last_ptr->parent.lock();
								if (parent_ptr && parent_ptr->show)
								{
									parent_ptr->color_u32 = ImGui::GetColorU32(ct::toImVec4(GraphView::GetRainBow((int32_t)visible_idx, (int32_t)visible_count)));
									parent_ptr->color_v4 = ImGui::ColorConvertU32ToFloat4(parent_ptr->color_u32);

									++visible_idx;
								}
							}
						}
						else
						{
							CTOOL_DEBUG_BREAK;
						}

						break;
					}
						
					last_ptr = ptr;
				}

				++idx;
			}
		}
	}
}

int SignalsPreview::CalcSignalsButtonCountAndSize(
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


int SignalsPreview::DrawSignalButton(int& vWidgetPushId, SignalTickPtr vPtr, ImVec2 vGlyphSize)
{
	int res = 0;

	if (vPtr)
	{
		auto parent_ptr = vPtr->parent.lock();
		if (parent_ptr)
		{
			ImGuiWindow* window = ImGui::GetCurrentWindow();
			if (window->SkipItems)
				return false;

			ImGuiContext& g = *GImGui;
			const ImGuiStyle& style = g.Style;

			ImGui::PushID(++vWidgetPushId);
			const ImGuiID id = window->GetID("#image");
			ImGui::PopID();

			ImRect bb(window->DC.CursorPos, window->DC.CursorPos + vGlyphSize + style.FramePadding * 2);
			ImGui::ItemSize(bb);
			if (!ImGui::ItemAdd(bb, id))
				return false;

			bool hovered, held;
			ImGui::ButtonBehavior(bb, id, &hovered, &held, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);
			if (held)
			{
				if (g.ActiveIdMouseButton == 0) // left click
					res = 1;
				if (g.ActiveIdMouseButton == 1) // right click
					res = 2;
			}

			// Render

			double ratio = (vPtr->value - parent_ptr->range_value.x) / (parent_ptr->range_value.y - parent_ptr->range_value.x);
			
			static auto ref_color = ImGui::GetColorU32(ImVec4(0, 0, 0, 1));
			ImVec4 target_color = parent_ptr->color_u32 != ref_color ? parent_ptr->color_v4 : ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
			ImVec4 color = ImLerp(ImVec4(0.1f, 0.1f, 0.1f, 1.0f), target_color, (float)ratio);

			const ImU32 colButton = ImGui::GetColorU32(color);
			const float rounding = ImClamp((float)ImMin(style.FramePadding.x, style.FramePadding.y), 0.0f, style.FrameRounding);

			// normal
			ImGui::RenderFrame(bb.Min, bb.Max, colButton, true, rounding);

			if (ImGui::IsItemHovered())
			{
				auto date_str = LogEngine::sConvertEpochToDateTimeString(vPtr->time_epoch);
				ImGui::SetTooltip("%s : %f\ntime : %f\ndate : %s", parent_ptr->name.c_str(), vPtr->value, vPtr->time_epoch, date_str.c_str());
			}
		}
	}

	return res;
}

void SignalsPreview::DrawTable()
{
	if (ImGui::BeginMenuBar())
	{
		float aw = ImGui::GetContentRegionAvail().x;

		//ImGui::SliderUIntDefaultCompact(aw, "Count buttons x", &ProjectFile::Instance()->m_SignalPreview_CountX, 1U, 1000U, 20U);
		ImGui::SliderFloat("Button Width", &ProjectFile::Instance()->m_SignalPreview_SizeX, 10.0f, 100.0f);

		ImGui::EndMenuBar();
	}

	auto win = ImGui::GetCurrentWindowRead();
	if (win)
	{
		const auto& signals_count = m_PreviewTicks.size();
		if (signals_count)
		{
			ImVec2 cell_size, button_size;
			const auto& signals_max_count_x = CalcSignalsButtonCountAndSize(cell_size, button_size);
			if (signals_max_count_x)
			{
				const int& rowCount = (int)ct::ceil((double)signals_count / (double)signals_max_count_x);

				uint32_t idx = 0U;
				m_VirtualClipper.Begin(rowCount, cell_size.y);
				while (m_VirtualClipper.Step())
				{
					for (int j = m_VirtualClipper.DisplayStart; j < m_VirtualClipper.DisplayEnd; ++j)
					{
						if (j < 0) continue;

						for (uint32_t i = 0; i < (uint32_t)signals_max_count_x; ++i)
						{
							uint32_t tick_idx = i + j * signals_max_count_x;
							if (tick_idx < signals_count)
							{
								auto ptr = m_PreviewTicks[tick_idx].lock();
								if (ptr)
								{
									uint32_t x = idx % signals_max_count_x;

									if (x) ImGui::SameLine();

									DrawSignalButton(SourcePane_WidgetId, ptr, button_size);

									++idx;
								}
							}
						}
					}
				}
				m_VirtualClipper.End();
			}
		}
	}
}
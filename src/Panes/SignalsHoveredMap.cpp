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

#include "SignalsHoveredMap.h"
#include <Project/ProjectFile.h>
#include <cinttypes> // printf zu
#include <panes/CodePane.h>

#include <models/lua/LuaEngine.h>
#include <models/log/LogEngine.h>
#include <models/log/SignalSerie.h>
#include <models/log/SignalTick.h>

static int SourcePane_WidgetId = 0;

///////////////////////////////////////////////////////////////////////////////////
//// IMGUI PANE ///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////

void SignalsHoveredMap::Clear()
{

}

bool SignalsHoveredMap::Init()
{
	return true;
}

void SignalsHoveredMap::Unit()
{

}

bool SignalsHoveredMap::DrawPanes(const uint32_t& /*vCurrentFrame*/, bool* vOpened, ImGuiContext* vContextPtr, void* /*vUserDatas*/) {
    ImGui::SetCurrentContext(vContextPtr);
    bool change = false;
    if (vOpened != nullptr && *vOpened) {
        static ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_MenuBar;
        if (ImGui::Begin(GetName().c_str(), vOpened, flags)) {
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

int SignalsHoveredMap::CalcSignalsButtonCountAndSize(
	ImVec2& vOutCellSize,					/* cell size						*/
	ImVec2& vOutButtonSize)					/* button size (cell - paddings)	*/
{
	float aw = ImGui::GetContentRegionAvail().x;

	float width = ProjectFile::Instance()->m_SignalPreview_SizeX;

    int count = (int)(aw / ez::maxi(width, 1.0f));
	width = aw / (float)ez::maxi(count, 1);

	ProjectFile::Instance()->m_SignalPreview_CountX = count;

	if (count > 0)
	{
		vOutCellSize = ImVec2(width, width);
		vOutButtonSize = vOutCellSize - ImGui::GetStyle().ItemSpacing - ImGui::GetStyle().FramePadding * 2.0f;
	}

	return count;
}


int SignalsHoveredMap::DrawSignalButton(int& vWidgetPushId, const SignalTickPtr& vPtr, ImVec2 vGlyphSize)
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

void SignalsHoveredMap::DrawTable()
{
	if (ImGui::BeginMenuBar())
	{
		float aw = ImGui::GetContentRegionAvail().x;

		ImGui::SliderFloat("Button Width", &ProjectFile::Instance()->m_SignalPreview_SizeX, 10.0f, 100.0f);

		ImGui::EndMenuBar();
	}

	auto win = ImGui::GetCurrentWindowRead();
	if (win)
	{
		const auto& signals_count = LogEngine::Instance()->GetPreviewTicks().size();
		if (signals_count)
		{
			ImVec2 cell_size, button_size;
			const auto& signals_max_count_x = CalcSignalsButtonCountAndSize(cell_size, button_size);
			if (signals_max_count_x)
			{
				const int& rowCount = (int)ez::ceil((double)signals_count / (double)signals_max_count_x);

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
								auto ptr = LogEngine::Instance()->GetPreviewTicks().at(tick_idx).lock();
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
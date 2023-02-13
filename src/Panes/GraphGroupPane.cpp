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

#include "GraphGroupPane.h"
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

#include <Engine/Lua/LuaEngine.h>
#include <Engine/Log/LogEngine.h>
#include <Engine/Log/SignalSerie.h>
#include <Engine/Log/SignalTick.h>

#include <Engine/Graphs/GraphView.h>

static int SourcePane_WidgetId = 0;

///////////////////////////////////////////////////////////////////////////////////
//// IMGUI PANE ///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////

bool GraphGroupPane::Init()
{
	return true;
}

void GraphGroupPane::Unit()
{

}

int GraphGroupPane::DrawPanes(const uint32_t& /*vCurrentFrame*/, const int& vWidgetId, const std::string& /*vvUserDatas*/, PaneFlag& vInOutPaneShown)
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
				GraphView::Instance()->DrawGraphGroupTable();
			}
		}

		ImGui::End();
	}

	return SourcePane_WidgetId;
}

void GraphGroupPane::DrawDialogsAndPopups(const uint32_t& /*vCurrentFrame*/, const std::string& /*vvUserDatas*/)
{
	
}

int GraphGroupPane::DrawWidgets(const uint32_t& /*vCurrentFrame*/, const int& vWidgetId, const std::string& /*vvUserDatas*/)
{
	return vWidgetId;
}
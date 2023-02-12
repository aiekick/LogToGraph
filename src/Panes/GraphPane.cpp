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

#include "GraphPane.h"
#include <Gui/MainFrame.h>
#include <ctools/cTools.h>
#include <ctools/Logger.h>
#include <Helper/Messaging.h>
#include <Project/ProjectFile.h>
#include <Panes/Manager/LayoutManager.h>
#include <cinttypes> // printf zu
#include <Engine/Graphs/GraphView.h>
#include <Engine/Graphs/GraphGroup.h>
#include <imgui/imgui_internal.h>
#include <Engine/Log/LogEngine.h>

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

int GraphPane::DrawPanes(const uint32_t& /*vCurrentFrame*/, const int& vWidgetId, const std::string& /*vvUserDatas*/, PaneFlag& vInOutPaneShown)
{
	GeneratorPaneWidgetId = vWidgetId;

	if (vInOutPaneShown & m_PaneFlag)
	{
		auto& graphGroups = GraphView::Instance()->GetGraphGroups();

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
				if (ImGui::BeginMenuBar())
				{
					GraphView::Instance()->DrawMenuBar();

					ImGui::EndMenuBar();
				}

				if (!graphGroups.empty())
				{
					// on compte le nombre de graphs
					uint32_t count_graphs = GraphView::Instance()->GetGraphCount();

					// on calcule la taille de chaque graphs
					ImVec2 amh = ImGui::GetContentRegionAvail();

					if (count_graphs)
					{
						amh.y -= ImGui::GetStyle().ItemInnerSpacing.y * (float)(count_graphs - 1U);
						amh.y /= (float)count_graphs;
					}

					// on les affichent
					if (ImPlot::BeginAlignedPlots("AlignedGroup"))
					{
						bool first_graph = true;

						for (auto it = graphGroups.begin(); it != graphGroups.end(); ++it)
						{
							if (it == graphGroups.begin())
							{
								GraphView::Instance()->DrawAloneGraphs(*it, amh, first_graph);
							}
							else
							{
								GraphView::Instance()->DrawGroupedGraphs(*it, amh, first_graph);
							}
						}

						ImPlot::EndAlignedPlots();
					}
				}
			}
		}

		ImGui::End();
	}

	return GeneratorPaneWidgetId;
}

void GraphPane::DrawDialogsAndPopups(const uint32_t& /*vCurrentFrame*/, const std::string& /*vvUserDatas*/)
{

}

int GraphPane::DrawWidgets(const uint32_t& /*vCurrentFrame*/, const int& vWidgetId, const std::string& /*vvUserDatas*/)
{
	return vWidgetId;
}

void GraphPane::DoVirtualLayout()
{
	auto window_ptr = ImGui::FindWindowByName(m_PaneName.c_str());
	if (window_ptr)
	{
		auto& graphGroups = GraphView::Instance()->GetGraphGroups();
		if (graphGroups.size() > 2U)
		{
			auto start_gg_it = graphGroups.begin(); ++start_gg_it;
			auto end_gg_it = graphGroups.end(); --end_gg_it; --end_gg_it;
			for (auto ggIt = start_gg_it; ggIt != end_gg_it; ++ggIt)
			{
				auto graph_group_ptr = *ggIt;
				if (graph_group_ptr)
				{
					ImGui::DockBuilderDockWindow(graph_group_ptr->GetImGuiLabel(), window_ptr->DockId);
				}
			}
		}
	}
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

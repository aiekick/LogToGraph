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
#include <cinttypes> // printf zu
#include <Engine/Graphs/GraphView.h>
#include <Engine/Graphs/GraphGroup.h>
#include <imgui/imgui_internal.h>

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

int GraphPane::DrawPanes(const uint32_t& /*vCurrentFrame*/, int vWidgetId, std::string /*vUserDatas*/, PaneFlag& vInOutPaneShown)
{
	GeneratorPaneWidgetId = vWidgetId;

	if (vInOutPaneShown & m_PaneFlag)
	{
		auto& graphGroups = GraphView::Instance()->GetGraphGroups();
		
		bool _need_re_layout = false;

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
					if (ImGui::MenuItem("Default Layout"))
					{
						_need_re_layout = true;
					}

					GraphView::Instance()->DrawMenuBar();

					ImGui::EndMenuBar();
				}

				if (!graphGroups.empty())
				{
					GraphView::Instance()->DrawAloneGraphs(*graphGroups.begin(), ImVec2(-1.0f,100.0f));
				}
			}
		}

		ImGui::End();

		if (graphGroups.size() > 2U)
		{
			auto start_gg_it = ++graphGroups.begin();
			auto end_gg_it = --graphGroups.end();
			for (auto ggIt = start_gg_it; ggIt != end_gg_it; ++ggIt)
			{
				auto graph_group_ptr = *ggIt;
				if (graph_group_ptr)
				{
					if (ImGui::Begin(graph_group_ptr->GetName(), nullptr, flags))
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

							GraphView::Instance()->DrawGroupedGraphs(graph_group_ptr, ImVec2(-1.0f, -1.0f));
						}
					}

					ImGui::End();
				}
			}
		}

		if (_need_re_layout)
		{
			DoVirtualLayout();
		}
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
					ImGui::DockBuilderDockWindow(graph_group_ptr->GetName(), window_ptr->DockId);
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

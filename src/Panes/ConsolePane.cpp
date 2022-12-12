// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include "ConsolePane.h"

#include <Gui/MainFrame.h>
#include <imgui/imgui_internal.h>

#include <ctools/cTools.h>
#include <ctools/Logger.h>
#include <Helper/Messaging.h>

#include <Project/ProjectFile.h>

#include <cinttypes> // printf zu

static int GeneratorPaneWidgetId = 0;

///////////////////////////////////////////////////////////////////////////////////
//// OVERRIDES ////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////

bool ConsolePane::Init()
{
	return true;
}

void ConsolePane::Unit()
{

}

int ConsolePane::DrawPanes(const uint32_t& /*vCurrentFrame*/, int vWidgetId, std::string /*vUserDatas*/, PaneFlag& vInOutPaneShown)
{
	GeneratorPaneWidgetId = vWidgetId;

	if (vInOutPaneShown & m_PaneFlag)
	{
		static ImGuiWindowFlags flags =
			ImGuiWindowFlags_NoCollapse |
			ImGuiWindowFlags_NoBringToFrontOnFocus |
			ImGuiWindowFlags_MenuBar;
		if (ImGui::Begin<PaneFlag>(m_PaneName,
			&vInOutPaneShown, m_PaneFlag, flags))
		{
#ifdef USE_DECORATIONS_FOR_RESIZE_CHILD_WINDOWS
			auto win = ImGui::GetCurrentWindowRead();
			if (win->Viewport->Idx != 0)
				flags |= ImGuiWindowFlags_NoResize;// | ImGuiWindowFlags_NoTitleBar;
			else
				flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_MenuBar;
#endif
			if (ProjectFile::Instance()->IsLoaded()) {}

			if (ImGui::BeginMenuBar())
			{
#ifdef WIN32
				/*if (ImGui::MenuItem("Console", "", &MainBackend::Instance()->puConsoleVisiblity))
				{
					MainBackend::Instance()->SetConsoleVisibility(MainBackend::Instance()->puConsoleVisiblity);
				}*/
#endif
				ImGui::EndMenuBar();
			}

			Messaging::Instance()->DrawConsole();
		}

		ImGui::End();
	}

	return GeneratorPaneWidgetId;
}

void ConsolePane::DrawDialogsAndPopups(const uint32_t& /*vCurrentFrame*/, std::string /*vUserDatas*/)
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

int ConsolePane::DrawWidgets(const uint32_t& /*vCurrentFrame*/, int vWidgetId, std::string /*vUserDatas*/)
{
	return vWidgetId;
}

std::string ConsolePane::getXml(const std::string& /*vOffset*/, const std::string& /*vUserDatas*/)
{
	std::string str;

	return str;
}

bool ConsolePane::setFromXml(tinyxml2::XMLElement* vElem, tinyxml2::XMLElement* vParent, const std::string& /*vUserDatas*/)
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


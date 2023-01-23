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

#include "CodePane.h"
#include <Gui/MainFrame.h>
#include <ctools/cTools.h>
#include <ctools/Logger.h>
#include <Panes/ToolPane.h>
#include <Helper/Messaging.h>
#include <ctools/FileHelper.h>
#include <Project/ProjectFile.h>
#include <imgui/imgui_internal.h>
#include <Panes/Manager/LayoutManager.h>
#include <Contrib/ImWidgets/ImWidgets.h>
#include <Engine/Lua/LuaEngine.h>

#include <cinttypes> // printf zu

static int GeneratorPaneWidgetId = 0;

///////////////////////////////////////////////////////////////////////////////////
//// OVERRIDES ////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////

bool CodePane::Init()
{
	m_CodeEditor.SetLanguageDefinition(TextEditor::LanguageDefinition::Lua());
	m_CodeEditor.SetPalette(TextEditor::GetDarkPalette());

	return true;
}

void CodePane::Unit()
{

}

int CodePane::DrawPanes(const uint32_t& /*vCurrentFrame*/, const int& vWidgetId, const std::string& /*vvUserDatas*/, PaneFlag& vInOutPaneShown)
{
	GeneratorPaneWidgetId = vWidgetId;

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
				flags |= ImGuiWindowFlags_NoResize;// | ImGuiWindowFlags_NoTitleBar;
			else
				flags = ImGuiWindowFlags_NoCollapse |
				ImGuiWindowFlags_NoBringToFrontOnFocus |
				ImGuiWindowFlags_MenuBar;
#endif
			if (ProjectFile::Instance()->IsLoaded()) 
			{
				DrawEditor();
			}
		}

		//MainFrame::sAnyWindowsHovered |= ImGui::IsWindowHovered();

		ImGui::End();
	}

	return GeneratorPaneWidgetId;
}

void CodePane::DrawDialogsAndPopups(const uint32_t& /*vCurrentFrame*/, const std::string& /*vvUserDatas*/)
{
	ImVec2 maxSize = MainFrame::Instance()->m_DisplaySize;
	ImVec2 minSize = maxSize * 0.5f;

	if (ImGuiFileDialog::Instance()->Display("OpenLuaScript",
		ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking,
		minSize, maxSize))
	{
		if (ImGuiFileDialog::Instance()->IsOk())
		{
			ProjectFile::Instance()->m_CodeFilePathName = ImGuiFileDialog::Instance()->GetFilePathName();
			SetCode(FileHelper::Instance()->LoadFileToString(ProjectFile::Instance()->m_CodeFilePathName));
		}

		ImGuiFileDialog::Instance()->Close();
	}

	if (ImGuiFileDialog::Instance()->Display("SaveLuaScript",
		ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking,
		minSize, maxSize))
	{
		if (ImGuiFileDialog::Instance()->IsOk())
		{
			ProjectFile::Instance()->m_CodeFilePathName = ImGuiFileDialog::Instance()->GetFilePathName();
			FileHelper::Instance()->SaveStringToFile(GetCode(), ProjectFile::Instance()->m_CodeFilePathName);
		}

		ImGuiFileDialog::Instance()->Close();
	}
}

int CodePane::DrawWidgets(const uint32_t& /*vCurrentFrame*/, const int& vWidgetId, const std::string& /*vvUserDatas*/)
{
	return vWidgetId;
}

void CodePane::SetCodeFile(const std::string& vFile)
{
	SetCode(FileHelper::Instance()->LoadFileToString(vFile));
}

void CodePane::SetCode(const std::string& vCode)
{
	m_CodeEditor.SetText(vCode);
}

std::string CodePane::GetCode()
{
	return m_CodeEditor.GetText();
}

std::string CodePane::getXml(const std::string& /*vOffset*/, const std::string& /*vUserDatas*/)
{
	std::string str;

	return str;
}

bool CodePane::setFromXml(tinyxml2::XMLElement* vElem, tinyxml2::XMLElement* vParent, const std::string& /*vUserDatas*/)
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

void CodePane::DrawEditor()
{
	if (ImGui::BeginMenuBar())
	{
		if (ImGui::MenuItem("Open", "Open Lua Script"))
		{
			ImGuiFileDialog::Instance()->OpenDialog("OpenLuaScript", "Open Lua Script", ".lua, .*", "");
		}

		if (ImGui::MenuItem("Save", "Save Lua Script"))
		{
			if (ProjectFile::Instance()->m_CodeFilePathName.empty() || 
				!FileHelper::Instance()->IsFileExist(ProjectFile::Instance()->m_CodeFilePathName))
			{
				ImGuiFileDialog::Instance()->OpenDialog("SaveLuaScript", "Save Lua Script", ".lua", "");
			}
			else
			{
				FileHelper::Instance()->SaveStringToFile(GetCode(), ImGuiFileDialog::Instance()->GetFilePathName());
			}
		}

		if (ImGui::MenuItem("Save As", "Save Lua Script"))
		{
			ImGuiFileDialog::Instance()->OpenDialog("SaveLuaScript", "Save Lua Script", ".lua", "");
		}

		if (ImGui::MenuItem("Clear", "Clear Shader Code"))
		{
			m_CodeEditor.SetText("");
		}

		if (ImGui::MenuItem("Run"))
		{
			ExecCode();
		}

		ImGui::EndMenuBar();
	}

	m_CodeEditor.Render("Code", ImVec2(-1, -1), false);

	if (m_CodeEditor.IsTextChanged()) // is user write code
	{
		ProjectFile::Instance()->SetProjectChange();
	}
}

void CodePane::ExecCode()
{
	std::string errors;
	LuaEngine::Instance()->ExecScriptCode(GetCode(), errors);

	auto arr = ct::splitStringToVector(errors, '\n');
	for (auto a : arr)
	{
		LogVarLightError("%s", a.c_str());
	}
}
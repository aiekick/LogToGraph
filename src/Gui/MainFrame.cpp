// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/*
 * Copyright 2022-2023 Stephane Cuillerdier (aka Aiekick)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "MainFrame.h"

#include <ctools/cTools.h>
#include <ctools/FileHelper.h>

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <Helper/Messaging.h>
#include <ImGuiFileDialog/ImGuiFileDialog.h>
#include <Contrib/ImWidgets/ImWidgets.h>
#include <Panes/Manager/LayoutManager.h>
#include <Helper/ThemeHelper.h>
#include <Project/ProjectFile.h>
#include <Contrib/FontIcons/CustomFont.h>

#include <Headers/Globals.h>
#include <Headers/LogToGraphBuild.h>

#include <Contrib/FontIcons/CustomFontToolBar.h>

#include <imgui/imgui_internal.h>

#include <Panes/LogPane.h>
#include <Panes/CodePane.h>
#include <Panes/ToolPane.h>
#include <Panes/GraphPane.h>
#include <Panes/ConsolePane.h>
#include <Panes/GraphGroupPane.h>
#include <Panes/SignalsHoveredDiff.h>
#include <Panes/SignalsHoveredList.h>
#include <Panes/SignalsHoveredMap.h>
#include <Panes/LogPaneSecondView.h>
#include <Panes/GraphListPane.h>
#include <Panes/AnnotationPane.h>

#include <Engine/Lua/LuaEngine.h>

#include <implot/implot.h>

#define WIDGET_ID_MAGIC_NUMBER 4577

MainFrame::MainFrame(GLFWwindow *vWin)
{
	m_Window = vWin;
}

MainFrame::~MainFrame() = default;

void MainFrame::Init()
{
	SetAppTitle();

	LayoutManager::Instance()->Init(ICON_NDPTB_DESKTOP_MAC " Layouts", "Default Layout");

	LayoutManager::Instance()->SetPaneDisposalSize(PaneDisposal::LEFT, 200.0f);
	LayoutManager::Instance()->SetPaneDisposalSize(PaneDisposal::RIGHT, 500.0f);
	LayoutManager::Instance()->SetPaneDisposalSize(PaneDisposal::BOTTOM, 200.0f);

	LayoutManager::Instance()->AddPane(ToolPane::Instance(), ICON_NDP2_CUBE_SCAN " Tool", "", PaneDisposal::LEFT, true, true);
	LayoutManager::Instance()->AddPane(LogPane::Instance(), ICON_NDP2_FILE_DOCUMENT_BOX " Logs", "", PaneDisposal::RIGHT, true, false);
	LayoutManager::Instance()->AddPane(LogPaneSecondView::Instance(), ICON_NDP2_FILE_DOCUMENT_BOX " Logs Second View", "", PaneDisposal::RIGHT, false, false);
	LayoutManager::Instance()->AddPane(GraphPane::Instance(), ICON_NDP2_CHART_LINE " Graphs", "", PaneDisposal::CENTRAL, true, false);
	LayoutManager::Instance()->AddPane(GraphListPane::Instance(), ICON_NDP2_CHART_LINE " All Graph Signals", "", PaneDisposal::CENTRAL, false, false);
	LayoutManager::Instance()->AddPane(GraphGroupPane::Instance(), ICON_NDP2_BUFFER " Graph Groups", "", PaneDisposal::RIGHT, true, false);
	LayoutManager::Instance()->AddPane(SignalsHoveredList::Instance(), ICON_NDP2_RECENT_FILES " Signals Hovered List", "", PaneDisposal::RIGHT, false, false);
	LayoutManager::Instance()->AddPane(SignalsHoveredDiff::Instance(), ICON_NDP2_VECTOR_DIFFERENCE " Signals Hovered Diff", "", PaneDisposal::RIGHT, false, false);
	LayoutManager::Instance()->AddPane(ConsolePane::Instance(), ICON_NDP2_COMMENT_TEXT_MULTIPLE " Console", "", PaneDisposal::BOTTOM, false, false);
	LayoutManager::Instance()->AddPane(CodePane::Instance(), ICON_NDP2_COMMENT_TEXT " Code", "", PaneDisposal::RIGHT, false, false);
	LayoutManager::Instance()->AddPane(AnnotationPane::Instance(), ICON_NDP2_CARDS " Annotations", "", PaneDisposal::RIGHT, false, false);

    // todo : not sure if interested so maybe to remove
    // LayoutManager::Instance()->AddPane(SignalsHoveredMap::Instance(), ICON_NDP2_CARDS " Signals Hovered Map", "", PaneDisposal::BOTTOM, false, false);

    // ConsolePane have a flag only after AddPane() call
	Messaging::sMessagePaneId = ConsolePane::Instance()->GetPaneFlag();
	Messaging::Instance();

	LayoutManager::Instance()->InitPanes();
	ThemeHelper::Instance(); // default theme

	SetEmbeddedIconApp("IDI_ICON1");
	//m_AppIconID = ExtractEmbeddedIcon("IDI_ICON1");
	m_BigAppIconID = ExtractEmbeddedImage("IDB_BMP1");

	LoadConfigFile("config.xml", "");

	ThemeHelper::Instance()->ApplyStyle();
	LuaEngine::Instance()->Init();

	LoadProject(m_ProjectToLoad);
}

void MainFrame::Unit()
{
	SaveConfigFile("config.xml", "");

	LuaEngine::Instance()->Unit();
	LayoutManager::Instance()->UnitPanes();

	// free opengl texture m_AppIconID
	if (m_AppIconID) { glDeleteTextures(GL_TEXTURE_2D, &m_AppIconID); m_AppIconID = 0U; }
	if (m_BigAppIconID) { glDeleteTextures(GL_TEXTURE_2D, &m_BigAppIconID); m_BigAppIconID = 0U; }
}

void MainFrame::NewProject(const std::string& vFilePathName)
{
	ProjectFile::Instance()->New(vFilePathName);
	SetAppTitle(vFilePathName);
}

void MainFrame::LoadProject(const std::string& vFilePathName)
{
	if (ProjectFile::Instance()->LoadAs(vFilePathName))
	{
		SetAppTitle(vFilePathName);
		ProjectFile::Instance()->SetProjectChange(false);
	}
	else if (!vFilePathName.empty())
	{
		Messaging::Instance()->AddError(true, nullptr, nullptr,
			"Failed to load project %s", vFilePathName.c_str());
	}
}

bool MainFrame::SaveProject()
{
	return ProjectFile::Instance()->Save();
}

void MainFrame::SaveAsProject(const std::string& vFilePathName)
{
	ProjectFile::Instance()->SaveAs(vFilePathName);

	if (m_NeedToCloseApp)
	{
		glfwSetWindowShouldClose(m_Window, GL_TRUE); // close app
	}
}

void MainFrame::SetEmbeddedIconApp(const char* vEmbeddedIconID)
{
#if WIN32
	auto icon_h_inst = LoadIconA(GetModuleHandle(NULL), vEmbeddedIconID);
	SetClassLongPtrA(glfwGetWin32Window(m_Window), GCLP_HICON, (LONG_PTR)icon_h_inst);
#else
    (void)vEmbeddedIconID;
#endif
}

GLuint MainFrame::ExtractEmbeddedIcon(const char* vEmbeddedIconID)
{
#if WIN32
	// embedded icon to opengl texture

	auto icon_h_inst = LoadIconA(GetModuleHandle(NULL), vEmbeddedIconID);

	ICONINFO app_icon_info;
	if (GetIconInfo(icon_h_inst, &app_icon_info))
	{
		HDC hdc = GetDC(0);

		BITMAPINFO MyBMInfo = { 0 };
		MyBMInfo.bmiHeader.biSize = sizeof(MyBMInfo.bmiHeader);
		if (GetDIBits(hdc, app_icon_info.hbmColor, 0, 0, NULL, &MyBMInfo, DIB_RGB_COLORS)) 
		{
			uint8_t* bytes = new uint8_t[MyBMInfo.bmiHeader.biSizeImage];

			MyBMInfo.bmiHeader.biCompression = BI_RGB;
			if (GetDIBits(hdc, app_icon_info.hbmColor, 0, MyBMInfo.bmiHeader.biHeight, (LPVOID)bytes, &MyBMInfo, DIB_RGB_COLORS))
			{
				uint8_t R, G, B;

				int index, i;

				// swap BGR to RGB
				for (i = 0; i < MyBMInfo.bmiHeader.biWidth * MyBMInfo.bmiHeader.biHeight; i++)
				{
					index = i * 4;

					B = bytes[index];
					G = bytes[index + 1];
					R = bytes[index + 2];

					bytes[index] = R;
					bytes[index + 1] = G;
					bytes[index + 2] = B;
				}

				//create texture from loaded bmp image 

				glEnable(GL_TEXTURE_2D);

				GLuint texID = 0;
				glGenTextures(1, &texID);

				glBindTexture(GL_TEXTURE_2D, texID);

				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

				glTexImage2D(GL_TEXTURE_2D, 0, 4, MyBMInfo.bmiHeader.biWidth, MyBMInfo.bmiHeader.biHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, bytes);
				glFinish();

				glBindTexture(GL_TEXTURE_2D, 0);

				glDisable(GL_TEXTURE_2D);

				return texID;
			}
		}
	}
#else
    (void)vEmbeddedIconID;
#endif

	return 0;
}

GLuint MainFrame::ExtractEmbeddedImage(const char* vEmbeddedImageID)
{
#if WIN32
	// embedded icon to opengl texture

	auto hBitmap = LoadImageA(GetModuleHandle(NULL), vEmbeddedImageID, IMAGE_BITMAP, 0, 0, LR_COPYFROMRESOURCE | LR_CREATEDIBSECTION);

	BITMAP bm;
	if (GetObjectA(hBitmap, sizeof(BITMAP), &bm))
	{
		uint8_t* bytes = (uint8_t*)bm.bmBits;
		uint8_t R, G, B;

		int index, i;

		// swap BGR to RGB
		for (i = 0; i < bm.bmWidth * bm.bmHeight; i++)
		{
			index = i * 3;

			B = bytes[index];
			G = bytes[index + 1];
			R = bytes[index + 2];

			bytes[index] = R;
			bytes[index + 1] = G;
			bytes[index + 2] = B;
		}

		//create texture from loaded bmp image 

		glEnable(GL_TEXTURE_2D);

		GLuint texID = 0;
		glGenTextures(1, &texID);

		glBindTexture(GL_TEXTURE_2D, texID);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, bm.bmWidth, bm.bmHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, bytes);
		glFinish();

		glBindTexture(GL_TEXTURE_2D, 0);

		glDisable(GL_TEXTURE_2D);

		return texID;
	}
#else
    (void)vEmbeddedImageID;
#endif

	return 0;
}

//////////////////////////////////////////////////////////////////////////////
//// DRAW ////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

void MainFrame::Display(ImVec2 vPos, ImVec2 vSize)
{
	m_DisplayPos = vPos;
	m_DisplaySize = vSize;

	const auto context = ImGui::GetCurrentContext();
	if (context)
	{
		ImGui::CustomStyle::Instance()->pushId = 4577;

		if (ImGui::BeginMainMenuBar())
		{
			DrawMainMenuBar();

			// ImGui Infos
			//auto& io = ImGui::GetIO();
			const auto label = ct::toStr("Dear ImGui %s (Docking)", ImGui::GetVersion());
			const auto size = ImGui::CalcTextSize(label.c_str());
			ImGui::Spacing(ImGui::GetContentRegionAvail().x - size.x - ImGui::GetStyle().FramePadding.x * 2.0f);
			ImGui::Text("%s", label.c_str());

			ImGui::EndMainMenuBar();
		}

		if (ImGui::BeginMainStatusBar())
		{
			Messaging::Instance()->DrawBar();

#ifdef _DEBUG
			// ImGui Infos
			const auto& io = ImGui::GetIO();
			const auto fps = ct::toStr("%.1f ms/frame (%.1f fps)", 1000.0f / io.Framerate, io.Framerate);
			const auto size = ImGui::CalcTextSize(fps.c_str());
			ImGui::Spacing(ImGui::GetContentRegionAvail().x - size.x - ImGui::GetStyle().FramePadding.x * 2.0f);
			ImGui::Text("%s", fps.c_str());
#endif

			ImGui::EndMainStatusBar();
		}

		if (LayoutManager::Instance()->BeginDockSpace(ImGuiDockNodeFlags_PassthruCentralNode))
		{
			//MainFrame::sCentralWindowHovered |= LayoutManager::Instance()->IsDockSpaceHoleHovered();

			LayoutManager::Instance()->EndDockSpace();
		}

        auto* cstyle = ImGui::CustomStyle::Instance();
        cstyle->pushId = LayoutManager::Instance()->DisplayPanes(0U, cstyle->pushId, "");

		DisplayDialogsAndPopups();

		ThemeHelper::Instance()->Draw();
		LayoutManager::Instance()->InitAfterFirstDisplay(vSize);

		if (m_ShowImGui)
			ImGui::ShowDemoWindow();
		if (m_ShowImPlot)
			ImPlot::ShowDemoWindow();
		if (m_ShowMetric)
			ImGui::ShowMetricsWindow(&m_ShowMetric);
	}
}

void MainFrame::DrawMainMenuBar()
{
	if (ImGui::BeginMenu(ICON_NDP_DOT_CIRCLE_O " Project"))
	{
		if (ImGui::MenuItem(ICON_NDP2_FILE " New"))
		{
			Action_Menu_NewProject();
		}

		if (ImGui::MenuItem(ICON_NDP_FOLDER_OPEN " Open"))
		{
			Action_Menu_OpenProject();
		}

		if (ProjectFile::Instance()->IsLoaded())
		{
			ImGui::Separator();

			if (ImGui::MenuItem(ICON_NDP_FOLDER_OPEN " Re Open"))
			{
				Action_Menu_ReOpenProject();
			}

			ImGui::Separator();

			if (ImGui::MenuItem(ICON_NDP_FLOPPY_O " Save"))
			{
				Action_Menu_SaveProject();
			}

			if (ImGui::MenuItem(ICON_NDP_FLOPPY_O " Save As"))
			{
				Action_Menu_SaveAsProject();
			}

			ImGui::Separator();

			if (ImGui::MenuItem(ICON_NDP_CANCEL " Close"))
			{
				Action_Menu_CloseProject();
			}
		}

		ImGui::Separator();

		if (ImGui::MenuItem(ICON_NDP_EXCLAMATION_CIRCLE " About"))
		{
			m_ShowAboutDialog = true;
		}

		ImGui::EndMenu();
	}

	ImGui::Spacing();

	LayoutManager::Instance()->DisplayMenu(m_DisplaySize);

	ImGui::Spacing();

	if (ImGui::BeginMenu(ICON_NDP_COG " Settings"))
	{
		if (ImGui::BeginMenu(ICON_NDP_PENCIL_SQUARE " Styles"))
		{
			ThemeHelper::Instance()->DrawMenu();

			ImGui::EndMenu();
		}

		ImGui::Separator();

		ImGui::MenuItem("Show ImGui", "", &m_ShowImGui);
		ImGui::MenuItem("Show ImPlot", "", &m_ShowImPlot);
		ImGui::MenuItem("Show ImGui Metric/Debug", "", &m_ShowMetric);

		ImGui::EndMenu();
	}

	if (ProjectFile::Instance()->IsThereAnyNotSavedChanged())
	{
		ImGui::Spacing(200.0f);

		if (ImGui::MenuItem(ICON_NDP_FLOPPY_O " Save"))
		{
			Action_Menu_SaveProject(); //-V1020
		} //-V1020
	} //-V1020
}

void MainFrame::OpenAboutDialog()
{
	m_ShowAboutDialog = true;
}

void MainFrame::DisplayDialogsAndPopups()
{
	m_ActionSystem.RunActions();

	if (ProjectFile::Instance()->IsLoaded())
	{
		LayoutManager::Instance()->DrawDialogsAndPopups(0U, "");
	}

	DrawAboutDialog();
}

void MainFrame::SetAppTitle(const std::string& vFilePathName)
{
	static char bufTitle[1024] = "";
	if (vFilePathName.empty())
	{
		snprintf(bufTitle, 1023, "%s %s", APP_TITLE, LogToGraph_BuildId);
		glfwSetWindowTitle(m_Window, bufTitle);
	}
	else
	{
		snprintf(bufTitle, 1023, "%s %s - Project : %s." APP_PROJECT_FILE_EXT, APP_TITLE, LogToGraph_BuildId, vFilePathName.c_str());
		glfwSetWindowTitle(m_Window, bufTitle);
	}
}

void MainFrame::DrawAboutDialog()
{
	if (m_ShowAboutDialog)
	{
		ImGui::OpenPopup("About");
		if (ImGui::BeginPopupModal("About", &m_ShowAboutDialog, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar))
		{
			ImGui::BeginGroup();

			// texture is inverted, so we invert uv.y
			ImGui::Image((ImTextureID)(void*)(size_t)m_BigAppIconID, ImVec2(128, 128), ImVec2(0,1), ImVec2(1,0));

			auto str = ct::toStr("%s %s", APP_TITLE, LogToGraph_BuildId);
			ImGui::ClickableTextUrl(str.c_str(), "https://github.com/aiekick/LogToGraph");

			ImGui::EndGroup();

			ImGui::SameLine();

			ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);

			ImGui::SameLine();

			ImGui::BeginGroup();

			ImGui::Text("License : %s", u8R"(
Copyright 2022-2023 Stephane Cuillerdier (aka aiekick)

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at)");
			ImGui::ClickableTextUrl("http://www.apache.org/licenses/LICENSE-2.0", "http://www.apache.org/licenses/LICENSE-2.0");

			ImGui::Text("%s", u8R"(Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
)");

			ImGui::Separator();

			ImGui::Text("%s", "Frameworks / Libraries used :");
			ImGui::Indent();
			{
				// glfw3
				ImGui::ClickableTextUrl("Glfw (ZLIB)", "https://github.com/glfw/glfw"); ImGui::SameLine();
				// glad
				ImGui::ClickableTextUrl("Glad (MIT)", "https://github.com/Dav1dde/glad"); ImGui::SameLine();
				// stb
				ImGui::ClickableTextUrl("Stb (MIT)", "https://github.com/nothings/stb"); ImGui::SameLine();
				// tinyxml2
				ImGui::ClickableTextUrl("tinyxml2 (ZLIB)", "https://github.com/leethomason/tinyxml2");
				// dirent
				ImGui::ClickableTextUrl("dirent (MIT)", "https://github.com/tronkko/dirent/blob/master/include/dirent.h"); ImGui::SameLine();
				// freetype 2
				ImGui::ClickableTextUrl("FreeType2 (FreeType2)", "https://github.com/freetype/freetype2"); ImGui::SameLine();
				// cTools
				ImGui::ClickableTextUrl("cTools (MIT)", "https://github.com/aiekick/cTools"); ImGui::SameLine();
				// LuaJit
				ImGui::ClickableTextUrl("Lua Jit (MIT)", "https://github.com/LuaJIT/LuaJIT");
				// BuildInc
				ImGui::ClickableTextUrl("BuildInc (MIT)", "https://github.com/aiekick/buildinc"); ImGui::SameLine();
				// ImGui
				ImGui::ClickableTextUrl("ImGui (MIT)", "https://github.com/ocornut/imgui"); ImGui::SameLine();
				// ImPlot
				ImGui::ClickableTextUrl("ImPlot (MIT)", "https://github.com/epezent/implot");
				// ImGui MarkDown
				ImGui::ClickableTextUrl("ImGui MarkDown (Zlib)", "https://github.com/juliettef/imgui_markdown"); ImGui::SameLine();
				// ImGuiColorTextEdit
				ImGui::ClickableTextUrl("ImGuiColorTextEdit (Zlib)", "https://github.com/BalazsJako/ImGuiColorTextEdit"); ImGui::SameLine();
				// ImGuiFileDialog
				ImGui::ClickableTextUrl("ImGuiFileDialog (MIT)", "https://github.com/aiekick/ImGuiFileDialog");
			}
			ImGui::Unindent();
			
			ImGui::EndGroup();

			ImGui::EndPopup();
		}
	}
}

///////////////////////////////////////////////////////
//// SAVE DIALOG WHEN UN SAVED CHANGES ////////////////
///////////////////////////////////////////////////////

void MainFrame::OpenUnSavedDialog()
{
	// force close dialog if any dialog is opened
	ImGuiFileDialog::Instance()->Close();

	m_SaveDialogIfRequired = true;
}

void MainFrame::CloseUnSavedDialog()
{
	m_SaveDialogIfRequired = false;
}

bool MainFrame::ShowUnSavedDialog()
{
	bool res = false;

	if (m_SaveDialogIfRequired)
	{
		if (ProjectFile::Instance()->IsLoaded())
		{
			if (ProjectFile::Instance()->IsThereAnyNotSavedChanged())
			{
				/*
				Unsaved dialog behavior :
				-	save :
					-	insert action : save project
				-	save as :
					-	insert action : save as project
				-	continue without saving :
					-	quit unsaved dialog
				-	cancel :
					-	clear actions
				*/

				ImGui::CloseCurrentPopup();
				ImGui::OpenPopup("Do you want to save before ?");
				if (ImGui::BeginPopupModal("Do you want to save before ?", (bool*)nullptr,
					ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoDocking))
				{
					if (ImGui::ContrastedButton("Save"))
					{
						res = Action_UnSavedDialog_SaveProject();
					}
					ImGui::SameLine();
					if (ImGui::ContrastedButton("Save As"))
					{
						Action_UnSavedDialog_SaveAsProject();
					}

					if (ImGui::ContrastedButton("Continue without saving"))
					{
						res = true; // quit the action
					}
					ImGui::SameLine();
					if (ImGui::ContrastedButton("Cancel"))
					{
						Action_Cancel();
					}

					ImGui::EndPopup();
				}
			}
		}

		return res; // quit if true, else continue on the next frame
	}

	return true; // quit the action
}

///////////////////////////////////////////////////////
//// ACTIONS //////////////////////////////////////////
///////////////////////////////////////////////////////

void MainFrame::Action_Menu_NewProject()
{
/*
new project :
-	unsaved :
	-	add action : show unsaved dialog
	-	add action : new project
-	saved :
	-	add action : new project
*/
	m_ActionSystem.Clear();
	m_ActionSystem.Add([this]()
		{
			CloseUnSavedDialog();
			ImGuiFileDialog::Instance()->OpenDialog(
				"NewProjectDlg", "New Project File", "Project File{." APP_PROJECT_FILE_EXT "}", ".", 1, nullptr, ImGuiFileDialogFlags_Modal);
			return true;
		});
	m_ActionSystem.Add([this]()
		{
			return Display_NewProjectDialog();
		});
}

void MainFrame::Action_Menu_OpenProject()
{
/*
open project : 
-	unsaved :
	-	add action : show unsaved dialog
	-	add action : open project
-	saved :
	-	add action : open project
*/
	m_ActionSystem.Clear();
	Action_OpenUnSavedDialog_IfNeeded();
	m_ActionSystem.Add([this]()
		{
			CloseUnSavedDialog(); 
			ImGuiFileDialog::Instance()->OpenDialog(
				"OpenProjectDlg", "Open Project File", "Project File{." APP_PROJECT_FILE_EXT "}", ".", 1, nullptr, ImGuiFileDialogFlags_Modal);
			return true;
		});
	m_ActionSystem.Add([this]()
		{
			return Display_OpenProjectDialog();
		});
}

void MainFrame::Action_Menu_ReOpenProject()
{
/*
re open project : 
-	unsaved :
	-	add action : show unsaved dialog
	-	add action : re open project
-	saved :
	-	add action : re open project
*/
	m_ActionSystem.Clear();
	Action_OpenUnSavedDialog_IfNeeded();
	m_ActionSystem.Add([this]()
		{
			LoadProject(ProjectFile::Instance()->m_ProjectFilePathName);
			return true;
		});
}

void MainFrame::Action_Menu_SaveProject()
{
/*
save project :
-	never saved :
	-	add action : save as project
-	saved in a file beofre :
	-	add action : save project
*/
	m_ActionSystem.Clear();
	m_ActionSystem.Add([this]()
		{
			if (!SaveProject())
			{
				CloseUnSavedDialog();
				ImGuiFileDialog::Instance()->OpenDialog(
					"SaveProjectDlg", "Save Project File", "Project File{." APP_PROJECT_FILE_EXT "}", ".", 1, nullptr, ImGuiFileDialogFlags_Modal);
			}
			return true;
		});
	m_ActionSystem.Add([this]()
		{
			return Display_SaveProjectDialog();
		});
}

void MainFrame::Action_Menu_SaveAsProject()
{
/*
save as project :
-	add action : save as project
*/
	m_ActionSystem.Clear();
	m_ActionSystem.Add([this]()
		{
			CloseUnSavedDialog();
			ImGuiFileDialog::Instance()->OpenDialog(
				"SaveProjectDlg", "Save Project File", "Project File{." APP_PROJECT_FILE_EXT "}", ".",
				1, nullptr, ImGuiFileDialogFlags_ConfirmOverwrite | ImGuiFileDialogFlags_Modal);
			return true;
		});
	m_ActionSystem.Add([this]()
		{
			return Display_SaveProjectDialog();
		});
}

void MainFrame::Action_Menu_CloseProject()
{
/*
Close project :
-	unsaved :
	-	add action : show unsaved dialog
	-	add action : Close project
-	saved :
	-	add action : Close project
*/
	m_ActionSystem.Clear();
	Action_OpenUnSavedDialog_IfNeeded();
	m_ActionSystem.Add([]()
		{
			ProjectFile::Instance()->Clear();
			return true;
		});
}

void MainFrame::Action_Window_CloseApp()
{
	if (m_NeedToCloseApp) return; // block next call to close app when running
/*
Close app :
-	unsaved :
	-	add action : show unsaved dialog
	-	add action : Close app
-	saved :
	-	add action : Close app
*/
	m_NeedToCloseApp = true;

	m_ActionSystem.Clear();
	Action_OpenUnSavedDialog_IfNeeded();
	m_ActionSystem.Add([this]()
		{
			glfwSetWindowShouldClose(m_Window, GL_TRUE); // close app
			return true;
		});
}

void MainFrame::Action_OpenUnSavedDialog_IfNeeded()
{
	if (ProjectFile::Instance()->IsLoaded() &&
		ProjectFile::Instance()->IsThereAnyNotSavedChanged())
	{
		OpenUnSavedDialog();
		m_ActionSystem.Add([this]()
			{
				return ShowUnSavedDialog();
			});
	}
}

void MainFrame::Action_Cancel()
{
/*
-	cancel :
	-	clear actions
*/
	CloseUnSavedDialog();
	m_ActionSystem.Clear();
	m_NeedToCloseApp = false;
}

bool MainFrame::Action_UnSavedDialog_SaveProject()
{
	bool res = SaveProject();
	if (!res)
	{
		m_ActionSystem.Insert([this]()
			{
				return Display_SaveProjectDialog();
			});
		m_ActionSystem.Insert([this]()
			{
				CloseUnSavedDialog();
				ImGuiFileDialog::Instance()->OpenDialog(
					"SaveProjectDlg", "Save Project File", "Project File{." APP_PROJECT_FILE_EXT "}",
					".", 1, nullptr, ImGuiFileDialogFlags_ConfirmOverwrite | ImGuiFileDialogFlags_Modal);
				return true;
			});
	}
	return res;
}

void MainFrame::Action_UnSavedDialog_SaveAsProject()
{
	m_ActionSystem.Insert([this]()
		{
			return Display_SaveProjectDialog();
		});
	m_ActionSystem.Insert([this]()
		{
			CloseUnSavedDialog();
			ImGuiFileDialog::Instance()->OpenDialog(
				"SaveProjectDlg", "Save Project File", "Project File{." APP_PROJECT_FILE_EXT "}",
				".", 1, nullptr, ImGuiFileDialogFlags_ConfirmOverwrite | ImGuiFileDialogFlags_Modal);
			return true;
		});
}

void MainFrame::Action_UnSavedDialog_Cancel()
{
	Action_Cancel();
}

///////////////////////////////////////////////////////
//// DIALOG FUNCS /////////////////////////////////////
///////////////////////////////////////////////////////

bool MainFrame::Display_NewProjectDialog()
{
	// need to return false to continue to be displayed next frame

	ImVec2 min = MainFrame::Instance()->m_DisplaySize * 0.5f;
	ImVec2 max = MainFrame::Instance()->m_DisplaySize;

	if (ImGuiFileDialog::Instance()->Display("NewProjectDlg",
		ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking, min, max))
	{
		if (ImGuiFileDialog::Instance()->IsOk())
		{
			CloseUnSavedDialog();
			auto ps = FileHelper::Instance()->ParsePathFileName(ImGuiFileDialog::Instance()->GetFilePathName());
			if (ps.isOk)
			{
				auto projectFilePathName = FileHelper::Instance()->ComposePath(ps.path, ps.name, APP_PROJECT_FILE_EXT);
				ProjectFile::Instance()->New(projectFilePathName);
				SaveAsProject(projectFilePathName);
			}
		}
		else // cancel
		{
			Action_Cancel(); // we interrupts all actions
		}

		ImGuiFileDialog::Instance()->Close();

		return true;
	}

	return false;
}

bool MainFrame::Display_OpenProjectDialog()
{
	// need to return false to continue to be displayed next frame

	ImVec2 min = MainFrame::Instance()->m_DisplaySize * 0.5f;
	ImVec2 max = MainFrame::Instance()->m_DisplaySize;
	
	if (ImGuiFileDialog::Instance()->Display("OpenProjectDlg",
		ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking, min, max))
	{
		if (ImGuiFileDialog::Instance()->IsOk())
		{
			CloseUnSavedDialog();
			LoadProject(ImGuiFileDialog::Instance()->GetFilePathName());
		}
		else // cancel
		{
			Action_Cancel(); // we interrupts all actions
		}

		ImGuiFileDialog::Instance()->Close();

		return true;
	}

	return false;
}

bool MainFrame::Display_SaveProjectDialog()
{
	// need to return false to continue to be displayed next frame

	ImVec2 min = MainFrame::Instance()->m_DisplaySize * 0.5f;
	ImVec2 max = MainFrame::Instance()->m_DisplaySize;
	
	if (ImGuiFileDialog::Instance()->Display("SaveProjectDlg",
		ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking, min, max))
	{
		if (ImGuiFileDialog::Instance()->IsOk())
		{
			CloseUnSavedDialog();
			SaveAsProject(ImGuiFileDialog::Instance()->GetFilePathName());
		}
		else // cancel
		{
			Action_Cancel(); // we interrupts all actions
		}

		ImGuiFileDialog::Instance()->Close();

		return true;
	}

	return false;
}

///////////////////////////////////////////////////////
//// APP CLOSING //////////////////////////////////////
///////////////////////////////////////////////////////

void MainFrame::IWantToCloseTheApp()
{
	Action_Window_CloseApp();
}

///////////////////////////////////////////////////////
//// DROP /////////////////////////////////////////////
///////////////////////////////////////////////////////

void MainFrame::JustDropFiles(int /*count*/, const char** /*paths*/)
{
	/*std::map<std::string, std::string> dicoFont;
	std::string prj;

	for (int i = 0; i < count; i++)
	{
		// file
		auto f = std::string(paths[i]);
		
		// lower case
		auto f_opt = f;
		for (auto& c : f_opt)
			c = (char)std::tolower((int)c);

		// well known extention
		if (	f_opt.find(".ttf") != std::string::npos			// truetype (.ttf)
			||	f_opt.find(".otf") != std::string::npos			// opentype (.otf)
			//||	f_opt.find(".ttc") != std::string::npos		// ttf/otf collection for futur (.ttc)
			)
		{
			dicoFont[f] = f;
		}
		if (f_opt.find(APP_PROJECT_FILE_EXT) != std::string::npos)
		{
			prj = f;
		}
	}

	// priority to project file
	if (!prj.empty())
	{
		LoadProject(prj);
	}*/
}

///////////////////////////////////////////////////////
//// CONFIGURATION ////////////////////////////////////
///////////////////////////////////////////////////////

std::string MainFrame::getXml(const std::string& vOffset, const std::string& vUserDatas)
{
	UNUSED(vUserDatas);

	std::string str;

	str += ThemeHelper::Instance()->getXml(vOffset, "app");
	str += LayoutManager::Instance()->getXml(vOffset, "app");
	str += vOffset + "<bookmarks>" + ImGuiFileDialog::Instance()->SerializeBookmarks() + "</bookmarks>\n";
	str += vOffset + "<showaboutdialog>" + (m_ShowAboutDialog ? "true" : "false") + "</showaboutdialog>\n";
	str += vOffset + "<showimgui>" + (m_ShowImGui ? "true" : "false") + "</showimgui>\n";
	str += vOffset + "<showmetric>" + (m_ShowMetric ? "true" : "false") + "</showmetric>\n";
	str += vOffset + "<project>" + ProjectFile::Instance()->m_ProjectFilePathName + "</project>\n";
	
	return str;
}

bool MainFrame::setFromXml(tinyxml2::XMLElement* vElem, tinyxml2::XMLElement* vParent, const std::string& vUserDatas)
{
	UNUSED(vUserDatas);

	// The value of this child identifies the name of this element
	std::string strName;
	std::string strValue;
	/*std::string strParentName;*/

	strName = vElem->Value();
	if (vElem->GetText())
		strValue = vElem->GetText();
	/*if (vParent != nullptr)
		strParentName = vParent->Value();*/

	ThemeHelper::Instance()->setFromXml(vElem, vParent, "app");
	LayoutManager::Instance()->setFromXml(vElem, vParent, "app");

	if (strName == "bookmarks")
		ImGuiFileDialog::Instance()->DeserializeBookmarks(strValue);
	else if (strName == "project")
		m_ProjectToLoad = strValue;
	else if (strName == "showaboutdialog")
		m_ShowAboutDialog = ct::ivariant(strValue).GetB();
	else if (strName == "showimgui")
		m_ShowImGui = ct::ivariant(strValue).GetB();
	else if (strName == "showmetric")
		m_ShowMetric = ct::ivariant(strValue).GetB();

	return true;
}

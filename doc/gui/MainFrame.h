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
#pragma once

#include <glad/glad.h>
#include <imgui/imgui.h>

#include <ctools/ConfigAbstract.h>
#include <Project/ProjectFile.h>
#include <Helper/FrameActionSystem.h>

#include <functional>
#include <string>
#include <vector>
#include <map>

/*
Actions behavior :

new project :
-	unsaved :
	-	add action : show unsaved dialog
	-	add action : new project
-	saved :
	-	add action : new project
open project : 
-	unsaved :
	-	add action : show unsaved dialog
	-	add action : open project
-	saved :
	-	add action : open project
re open project :
-	unsaved :
	-	add action : show unsaved dialog
	-	add action : re open project
-	saved :
	-	add action : re open project
save project :
-	never saved :
	-	add action : save as project
-	saved in a file beofre :
	-	add action : save project
save as project :
-	add action : save as project
Close project :
-	unsaved :
	-	add action : show unsaved dialog
	-	add action : Close project
-	saved :
	-	add action : Close project
Close app :
-	unsaved :
	-	add action : show unsaved dialog
	-	add action : Close app
-	saved :
	-	add action : Close app

Unsaved dialog behavior :
-	save : 
	-	insert action : save project
-	save as :
	-	insert action : save as project
-	continue without saving :
	-	quit unsaved dialog
-	cancel :
	-	clear actions

open font :
	-	add action : open font

close font :
-	ok :
	-	glyphs selected :
		-	add action : show a confirmation dialog (ok/cancel for lose glyph selection)
		-	add action : close font
	-	no glyph selected :
		-	add action : close font
-	cancel :
	-	clear actions

confirmation dialog for close font :
-	ok :
	-	quit the dialog
-	cancel :
	-	clear actions
*/

struct GLFWwindow;
class MainFrame : public conf::ConfigAbstract
{
public:
	bool leftMouseClicked = false;
	bool leftMouseReleased = false;
	bool rightMouseClicked = false;
	ImVec2 m_DisplayPos = ImVec2(0, 0); // viewport
	ImVec2 m_DisplaySize = ImVec2(1280, 720);

	bool m_ShowImGui = false;				// show ImGui win
	bool m_ShowImPlot = false;				// show ImPlot Demo
	bool m_ShowMetric = false;				// show metrics
	
private:
	bool m_ShowAboutDialog = false;			// show about dlg
	bool m_NeedToCloseApp = false;			// whenn app closing app is required
	bool m_SaveDialogIfRequired = false;	// open save options dialog (save / save as / continue without saving / cancel)
	FrameActionSystem m_ActionSystem;

	std::string m_ProjectToLoad;

	GLuint m_AppIconID = 0U;
	GLuint m_BigAppIconID = 0U;

public:
	void Init();
	void Unit();

	void NewProject(const std::string& vFilePathName);
	void LoadProject(const std::string& vFilePathName);
	static bool SaveProject();
	void SaveAsProject(const std::string& vFilePathName);

	void Display(ImVec2 vPos, ImVec2 vSize);

	GLFWwindow* GetGLFWwindow() { return m_Window; }
	FrameActionSystem* GetActionSystem() { return &m_ActionSystem; }

	void OpenAboutDialog();

	void SetEmbeddedIconApp(const char* vEmbeddedIconID);
	GLuint ExtractEmbeddedIcon(const char* vEmbeddedIconID);
	GLuint ExtractEmbeddedImage(const char* vEmbeddedImageID);

public: // save : on quit or project loading
	void IWantToCloseTheApp(); // user want close app, but we want to ensure its saved

public: // drop
	void JustDropFiles(int count, const char** paths);

private: // imgui pane / dialogs
	void DisplayDialogsAndPopups();
	void DrawMainMenuBar();
	void DrawAboutDialog();

private: // save : on quit or project loading
	void OpenUnSavedDialog(); // show a dialog because the project file is not saved
	void CloseUnSavedDialog(); // show a dialog because the project file is not saved
	bool ShowUnSavedDialog(); // show a dilaog because the project file is not saved
	
public: // actions
	// via menu
	void Action_Menu_NewProject();
	void Action_Menu_OpenProject();
	void Action_Menu_ReOpenProject();
	void Action_Menu_SaveProject();
	void Action_Menu_SaveAsProject();
	void Action_Menu_CloseProject();

private: // actions
	// view the window
	void Action_Window_CloseApp();

	// via the unsaved dialog
	bool Action_UnSavedDialog_SaveProject();
	void Action_UnSavedDialog_SaveAsProject();
	void Action_UnSavedDialog_Cancel();

	// others
	void Action_OpenUnSavedDialog_IfNeeded();
	void Action_Cancel();

	// dialog funcs to be in actionsbool 
	bool Display_NewProjectDialog();
	bool Display_OpenProjectDialog();
	bool Display_SaveProjectDialog();

private:
	void SetAppTitle(const std::string& vFilePathName = "");

public: // configuration
	std::string getXml(const std::string& vOffset, const std::string& vUserDatas);
	bool setFromXml(tinyxml2::XMLElement* vElem, tinyxml2::XMLElement* vParent, const std::string& vUserDatas);

public: // singleton
	static MainFrame *Instance(GLFWwindow *vWin = 0)
	{
		static MainFrame *_instance = new MainFrame(vWin);
		return _instance;
	}

protected:
	GLFWwindow *m_Window = 0;
	MainFrame(GLFWwindow *vWin); // Prevent construction
	MainFrame(const MainFrame&) {}; // Prevent construction by copying
	MainFrame& operator =(const MainFrame&) { return *this; }; // Prevent assignment
	~MainFrame(); // Prevent unwanted destruction
};


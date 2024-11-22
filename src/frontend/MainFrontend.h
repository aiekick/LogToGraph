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

#pragma once

#include <ImGuiPack.h>
#include <ezlibs/ezXmlConfig.hpp>

#include <Systems/FrameActionSystem.h>

#include <Backend/MainBackend.h>

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

class MainFrontend : public ez::xml::Config {
private:
    bool m_ShowImGui = false;
    bool m_ShowImPlot = false;
    bool m_ShowMetric = false;
    ImFont* m_ToolbarFontPtr = nullptr;
    ImVec2 m_DisplayPos = ImVec2(0, 0);  // viewport
    ImVec2 m_DisplaySize = ImVec2(1280, 720);
    bool m_ShowAboutDialog = false;          // show about dlg
    bool m_SaveDialogIfRequired = false;     // open save options dialog (save / save as / continue without saving / cancel)
    bool m_SaveDialogActionWasDone = false;  // if action was done by save options dialog
    FrameActionSystem m_ActionSystem;

public:
    static bool sCentralWindowHovered;

public:
    virtual ~MainFrontend();

    bool init();
    void unit();

    bool isValid() const;
    bool isThereAnError() const;

    void Display(const uint32_t& vCurrentFrame, const ImVec2& vpos, const ImVec2& vSize);

    bool DrawWidgets(const uint32_t& vCurrentFrame, ImGuiContext* vContextPtr, void* vUserDatas);
    bool DrawOverlays(const uint32_t& vCurrentFrame, const ImRect& vRect, ImGuiContext* vContextPtr, void* vUserDatas);
    bool DrawDialogsAndPopups(const uint32_t& vCurrentFrame, const ImRect& vMaxRect, ImGuiContext* vContextPtr, void* vUserDatas);

    void OpenAboutDialog();

    FrameActionSystem* GetActionSystem() {
        return &m_ActionSystem;
    }

public:                         // save : on quit or project loading
    void IWantToCloseTheApp();  // user want close app, but we want to ensure its saved

public:  // drop
    void JustDropFiles(int count, const char** paths);

private:                        // save : on quit or project loading
    void OpenUnSavedDialog();   // show a dialog because the project file is not saved
    void CloseUnSavedDialog();  // show a dialog because the project file is not saved
    bool ShowUnSavedDialog();   // show a dilaog because the project file is not saved

public:  // actions
    // via menu
    void Action_Menu_NewProject();
    void Action_Menu_OpenProject();
    void Action_Menu_ReOpenProject();
    void Action_Menu_SaveProject();
    void Action_Menu_SaveAsProject();
    void Action_Menu_CloseProject();
    void Action_Window_CloseApp();

private:  // actions
    // via the unsaved dialog
    bool Action_UnSavedDialog_SaveProject();
    void Action_UnSavedDialog_SaveAsProject();
    void Action_UnSavedDialog_Cancel();
    // others
    void Action_OpenUnSavedDialog_IfNeeded();
    void Action_Cancel();
    // dialog funcs to be in actions
    bool Display_NewProjectDialog();
    bool Display_OpenProjectDialog();
    bool Display_SaveProjectDialog();

public:  // configuration
    ez::xml::Nodes getXmlNodes(const std::string& vUserDatas = "") override;
    bool setFromXmlNodes(const ez::xml::Node& vNode, const ez::xml::Node& vParent, const std::string& vUserDatas) override;

private:
    bool m_build();
    bool m_build_themes();
    void m_drawMainMenuBar();
    void m_drawMainStatusBar();

public:  // singleton
    static MainFrontend* Instance() {
        static MainFrontend _instance;
        return &_instance;
    };
};

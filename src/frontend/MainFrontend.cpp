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

#include <Headers/DatasDef.h>

#include "MainFrontend.h"

#include <Backend/MainBackend.h>

#include <Project/ProjectFile.h>

#include <systems/PluginManager.h>

#include <panes/ConsolePane.h>
#include <panes/ProfilerPane.h>
#include <panes/CodePane.h>
#include <panes/LogPane.h>
#include <panes/CodePane.h>
#include <panes/ToolPane.h>
#include <panes/GraphPane.h>
#include <panes/ConsolePane.h>
#include <panes/GraphGroupPane.h>
#include <panes/SignalsHoveredDiff.h>
#include <panes/SignalsHoveredList.h>
#include <panes/SignalsHoveredMap.h>
#include <panes/LogPaneSecondView.h>
#include <panes/GraphListPane.h>
#include <panes/AnnotationPane.h>

#include <res/fontIcons.h>

#include <systems/SettingsDialog.h>

#include <systems/TranslationHelper.h>

#include <Headers/LogToGraphBuild.h>

// panes
#define DEBUG_PANE_ICON ICON_SDFM_BUG
#define SCENE_PANE_ICON ICON_SDFM_FORMAT_LIST_BULLETED_TYPE
#define TUNING_PANE_ICON ICON_SDFM_TUNE
#define CONSOLE_PANE_ICON ICON_SDFMT_COMMENT_TEXT_MULTIPLE

// features
#define GRID_ICON ICON_SDFMT_GRID
#define MOUSE_ICON ICON_SDFMT_MOUSE
#define CAMERA_ICON ICON_SDFMT_CAMCORDER
#define GIZMO_ICON ICON_SDFMT_AXIS_ARROW

using namespace std::placeholders;

//////////////////////////////////////////////////////////////////////////////////
//// STATIC //////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////

bool MainFrontend::sCentralWindowHovered = false;

//////////////////////////////////////////////////////////////////////////////////
//// PUBLIC //////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////

MainFrontend::~MainFrontend() = default;

bool MainFrontend::init() {
    ImGui::CustomStyle::Init();

    m_build_themes();

    LayoutManager::Instance()->Init(ICON_FONT_TABLET_DASHBOARD " Layouts", "Default Layout");

    LayoutManager::Instance()->SetPaneDisposalRatio("LEFT", 0.25f);
    LayoutManager::Instance()->SetPaneDisposalRatio("RIGHT", 0.25f);
    LayoutManager::Instance()->SetPaneDisposalRatio("BOTTOM", 0.25f);

    LayoutManager::Instance()->AddPane(CodePane::Instance(), ICON_FONT_CODE_BRACES " Code", "Misc", "RIGHT", 0.25f, false, false);
    LayoutManager::Instance()->AddPane(ConsolePane::Instance(), ICON_FONT_COMMENT_TEXT_MULTIPLE " Console", "Misc", "BOTTOM", 0.3f, false, false);
    LayoutManager::Instance()->AddPane(ProfilerPane::Instance(), ICON_FONT_CHART_DONUT_VARIANT " Profiler", "Misc", "BOTTOM", 0.3f, false, false);

    LayoutManager::Instance()->AddPane(AnnotationPane::Instance(), ICON_FONT_CARDS " Annotations", "", "RIGHT", 0.25f, false, false);
    LayoutManager::Instance()->AddPane(LogPane::Instance(), ICON_FONT_FILE_DOCUMENT_BOX " Logs", "", "RIGHT", 0.25f, true, false);
    LayoutManager::Instance()->AddPane(LogPaneSecondView::Instance(), ICON_FONT_FILE_DOCUMENT_BOX  " Logs 2nd", "", "RIGHT", 0.25f, false, false);
    LayoutManager::Instance()->AddPane(GraphPane::Instance(), ICON_FONT_CHART_LINE " Graphs", "", "CENTRAL", 0.25f, true, false);
    LayoutManager::Instance()->AddPane(GraphListPane::Instance(), ICON_FONT_CHART_LINE " All Graph Signals", "", "CENTRAL", 0.25f, false, false);
    LayoutManager::Instance()->AddPane(GraphGroupPane::Instance(), ICON_FONT_BUFFER " Graph Groups", "", "RIGHT", 0.25f, true, false);
    LayoutManager::Instance()->AddPane(SignalsHoveredList::Instance(), ICON_FONT_CACTUS " Signals Hovered List", "", "RIGHT", 0.25f, false, false);
    LayoutManager::Instance()->AddPane(SignalsHoveredDiff::Instance(), ICON_FONT_VECTOR_DIFFERENCE " Signals Hovered Diff", "", "RIGHT", 0.25f, false, false);
    LayoutManager::Instance()->AddPane(ToolPane::Instance(), ICON_FONT_CUBE_SCAN " Tool", "", "LEFT", 0.25f, true, true);

    // InitPänes is done in m_InitPanes, because a specific order is needed

    return m_build();
}

void MainFrontend::unit() {
    LayoutManager::Instance()->UnitPanes();
    auto pluginPanes = PluginManager::Instance()->getPluginPanes();
    for (auto& pluginPane : pluginPanes) {
        if (!pluginPane.pane.expired()) {
            LayoutManager::Instance()->RemovePane(pluginPane.name);
        }
    }
}

bool MainFrontend::isValid() const {
    return false;
}

bool MainFrontend::isThereAnError() const {
    return false;
}

void MainFrontend::Display(const uint32_t& vCurrentFrame, const ImVec2& vPos, const ImVec2& vSize) {
    const auto context_ptr = ImGui::GetCurrentContext();
    if (context_ptr != nullptr) {
        const auto& io = ImGui::GetIO();

        m_DisplayPos = vPos;
        m_DisplaySize = vSize;

        MainFrontend::sCentralWindowHovered = (ImGui::GetCurrentContext()->HoveredWindow == nullptr);
        ImGui::CustomStyle::ResetCustomId();

        // m_drawLeftButtonBar();
        m_drawMainMenuBar();
        m_drawMainStatusBar();

        if (LayoutManager::Instance()->BeginDockSpace(ImGuiDockNodeFlags_PassthruCentralNode)) {
            /*if (MainBackend::Instance()->GetBackendDatasRef().canWeTuneGizmo) {
                const auto viewport = ImGui::GetMainViewport();
                ImGuizmo::SetDrawlist(ImGui::GetCurrentWindow()->DrawList);
                ImGuizmo::SetRect(viewport->Pos.x, viewport->Pos.y, viewport->Size.x, viewport->Size.y);
                ImRect rc(viewport->Pos.x, viewport->Pos.y, viewport->Size.x, viewport->Size.y);
                DrawOverlays(vCurrentFrame, rc, context_ptr, {});
            }*/
            LayoutManager::Instance()->EndDockSpace();
        }

        if (LayoutManager::Instance()->DrawPanes(vCurrentFrame, context_ptr, {})) {
            ProjectFile::Instance()->SetProjectChange();
        }

        DrawDialogsAndPopups(vCurrentFrame, ImRect(m_DisplayPos, m_DisplaySize), context_ptr, {});

        ImGuiThemeHelper::Instance()->Draw();
        LayoutManager::Instance()->InitAfterFirstDisplay(io.DisplaySize);
    }
}

bool MainFrontend::DrawDialogsAndPopups(const uint32_t& vCurrentFrame, const ImRect& vMaxRect, ImGuiContext* vContextPtr, void* vUserDatas) {
    m_ActionSystem.RunActions();
    LayoutManager::Instance()->DrawDialogsAndPopups(vCurrentFrame, vMaxRect, vContextPtr, vUserDatas);
    if (m_ShowImGui) {
        ImGui::ShowDemoWindow(&m_ShowImGui);
    }
    if (m_ShowImPlot) {
        ImPlot::ShowDemoWindow(&m_ShowImPlot);
    }
    if (m_ShowMetric) {
        ImGui::ShowMetricsWindow(&m_ShowMetric);
    }
    SettingsDialog::Instance()->Draw();
    m_drawAboutDialog();
    return false;
}

void MainFrontend::m_drawAboutDialog() {
    if (m_ShowAboutDialog) {
        ImGui::OpenPopup("About");
        if (ImGui::BeginPopupModal("About", &m_ShowAboutDialog, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar)) {
            ImGui::BeginGroup();

            // texture is inverted, so we invert uv.y
            auto texID = (ImTextureID)(void*)(size_t)MainBackend::Instance()->getBigAppIconID();
            ImGui::Image(texID, ImVec2(128, 128), ImVec2(0, 1), ImVec2(1, 0));

            auto str = ez::str::toStr("%s %s", APP_TITLE, LogToGraph_BuildId);
            ImGui::ClickableTextUrl(str.c_str(), "https://github.com/aiekick/LogToGraph");

            ImGui::EndGroup();

            ImGui::SameLine();

            ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);

            ImGui::SameLine();

            ImGui::BeginGroup();

            ImGui::Text("License : %s",
                        u8R"(
Copyright 2022-2024 Stephane Cuillerdier (aka aiekick)

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at)");
            ImGui::ClickableTextUrl("http://www.apache.org/licenses/LICENSE-2.0", "http://www.apache.org/licenses/LICENSE-2.0");

            ImGui::Text("%s",
                        u8R"(Unless required by applicable law or agreed to in writing, software
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
                ImGui::ClickableTextUrl("Glfw (ZLIB)", "https://github.com/glfw/glfw");
                ImGui::SameLine();
                // glad
                ImGui::ClickableTextUrl("Glad (MIT)", "https://github.com/Dav1dde/glad");
                ImGui::SameLine();
                // stb
                ImGui::ClickableTextUrl("Stb (MIT)", "https://github.com/nothings/stb");
                ImGui::SameLine();
                // tinyxml2
                ImGui::ClickableTextUrl("tinyxml2 (ZLIB)", "https://github.com/leethomason/tinyxml2");
                // dirent
                ImGui::ClickableTextUrl("dirent (MIT)", "https://github.com/tronkko/dirent/blob/master/include/dirent.h");
                ImGui::SameLine();
                // freetype 2
                ImGui::ClickableTextUrl("FreeType2 (FreeType2)", "https://github.com/freetype/freetype2");
                ImGui::SameLine();
                // cTools
                ImGui::ClickableTextUrl("cTools (MIT)", "https://github.com/aiekick/cTools");
                ImGui::SameLine();
                // ScriptJit
                ImGui::ClickableTextUrl("Script Jit (MIT)", "https://github.com/ScriptJIT/ScriptJIT");
                // BuildInc
                ImGui::ClickableTextUrl("BuildInc (MIT)", "https://github.com/aiekick/buildinc");
                ImGui::SameLine();
                // ImGui
                ImGui::ClickableTextUrl("ImGui (MIT)", "https://github.com/ocornut/imgui");
                ImGui::SameLine();
                // ImPlot
                ImGui::ClickableTextUrl("ImPlot (MIT)", "https://github.com/epezent/implot");
                // ImGui MarkDown
                ImGui::ClickableTextUrl("ImGui MarkDown (Zlib)", "https://github.com/juliettef/imgui_markdown");
                ImGui::SameLine();
                // ImGuiColorTextEdit
                ImGui::ClickableTextUrl("ImGuiColorTextEdit (Zlib)", "https://github.com/BalazsJako/ImGuiColorTextEdit");
                ImGui::SameLine();
                // ImGuiFileDialog
                ImGui::ClickableTextUrl("ImGuiFileDialog (MIT)", "https://github.com/aiekick/ImGuiFileDialog");
            }
            ImGui::Unindent();

            ImGui::EndGroup();

            ImGui::EndPopup();
        }
    }
}

void MainFrontend::OpenAboutDialog() {
    m_ShowAboutDialog = true;
}

void MainFrontend::m_drawMainMenuBar() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu(ICON_FONT_ARCHIVE " Project")) {
            if (ImGui::MenuItem(ICON_FONT_FILE " New")) {
                Action_Menu_NewProject();
            }

            if (ImGui::MenuItem(ICON_FONT_FOLDER_OPEN " Open")) {
                Action_Menu_OpenProject();
            }

            if (ProjectFile::Instance()->IsProjectLoaded()) {
                ImGui::Separator();

                if (ImGui::MenuItem(ICON_FONT_FOLDER_OPEN " Re Open")) {
                    Action_Menu_ReOpenProject();
                }

                ImGui::Separator();

                if (ImGui::MenuItem(ICON_FONT_FLOPPY " Save")) {
                    Action_Menu_SaveProject();
                }

                if (ImGui::MenuItem(ICON_FONT_FLOPPY " Save As")) {
                    Action_Menu_SaveAsProject();
                }

                ImGui::Separator();

                if (ImGui::MenuItem(ICON_FONT_CLOSE" Close")) {
                    Action_Menu_CloseProject();
                }
            }

            ImGui::Separator();

            if (ImGui::MenuItem(ICON_FONT_INFORMATION " About")) {
                OpenAboutDialog();
            }

            ImGui::EndMenu();
        }

        ImGui::Spacing();

        const auto& io = ImGui::GetIO();
        LayoutManager::Instance()->DisplayMenu(io.DisplaySize);

        ImGui::Spacing();

        if (ImGui::BeginMenu(ICON_FONT_TUNE " Tools")) {
            if (ImGui::MenuItem(ICON_FONT_SETTINGS " Settings")) {
                SettingsDialog::Instance()->OpenDialog();
            }
            ImGui::Separator();
            if (ImGui::BeginMenu(ICON_FONT_PALETTE " Styles")) {
                ImGuiThemeHelper::Instance()->DrawMenu();

                ImGui::Separator();

                ImGui::MenuItem("Show ImGui", "", &m_ShowImGui);
                ImGui::MenuItem("Show ImGui Metric/Debug", "", &m_ShowMetric);
                ImGui::MenuItem("Show ImPlot", "", &m_ShowImPlot);

                ImGui::EndMenu();
            }

            ImGui::EndMenu();
        }

        if (ProjectFile::Instance()->IsThereAnyProjectChanges()) {
            ImGui::Spacing(200.0f);

            if (ImGui::MenuItem(ICON_FONT_FLOPPY " Save")) {
                Action_Menu_SaveProject();
            }
        }

        // ImGui Infos
        const auto label = ez::str::toStr("Dear ImGui %s (Docking)", ImGui::GetVersion());
        const auto size = ImGui::CalcTextSize(label.c_str());
        static float s_translation_menu_size = 0.0f;

        ImGui::Spacing(ImGui::GetContentRegionAvail().x - size.x - s_translation_menu_size - ImGui::GetStyle().FramePadding.x * 2.0f);
        ImGui::Text("%s", label.c_str());

        s_translation_menu_size = TranslationHelper::Instance()->DrawMenu();

        ImGui::EndMainMenuBar();
    }
}

void MainFrontend::m_drawMainStatusBar() {
    if (ImGui::BeginMainStatusBar()) {
        Messaging::Instance()->DrawStatusBar();

        //  ImGui Infos
        const auto& io = ImGui::GetIO();
        const auto fps = ez::str::toStr("%.1f ms/frame (%.1f fps)", 1000.0f / io.Framerate, io.Framerate);
        const auto size = ImGui::CalcTextSize(fps.c_str());
        ImGui::Spacing(ImGui::GetContentRegionAvail().x - size.x - ImGui::GetStyle().FramePadding.x * 2.0f);
        ImGui::Text("%s", fps.c_str());

        // MainFrontend::sAnyWindowsHovered |= ImGui::IsWindowHovered();

        ImGui::EndMainStatusBar();
    }
}

///////////////////////////////////////////////////////
//// SAVE DIALOG WHEN UN SAVED CHANGES ////////////////
///////////////////////////////////////////////////////

void MainFrontend::OpenUnSavedDialog() {
    // force close dialog if any dialog is opened
    ImGuiFileDialog::Instance()->Close();

    m_SaveDialogIfRequired = true;
}
void MainFrontend::CloseUnSavedDialog() {
    m_SaveDialogIfRequired = false;
}

bool MainFrontend::ShowUnSavedDialog() {
    bool res = false;

    if (m_SaveDialogIfRequired) {
        if (ProjectFile::Instance()->IsProjectLoaded()) {
            if (ProjectFile::Instance()->IsThereAnyProjectChanges()) {
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
                const char* label = "Save before closing ?";
                ImGui::OpenPopup(label);
                const auto& io = ImGui::GetIO();
                ImGui::SetNextWindowPos(m_DisplayPos + m_DisplaySize * 0.5f, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
                if (ImGui::BeginPopupModal(label, (bool*)0, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoDocking)) {
                    const auto& width = ImGui::CalcTextSize("Continue without saving").x + ImGui::GetStyle().ItemInnerSpacing.x;

                    if (ImGui::ContrastedButton("Save", nullptr, nullptr, width * 0.5f)) {
                        res = Action_UnSavedDialog_SaveProject();
                    }
                    ImGui::SameLine();
                    if (ImGui::ContrastedButton("Save As", nullptr, nullptr, width * 0.5f)) {
                        Action_UnSavedDialog_SaveAsProject();
                    }

                    if (ImGui::ContrastedButton("Continue without saving")) {
                        res = true;  // quit the action
                    }

                    if (ImGui::ContrastedButton("Cancel", nullptr, nullptr, width + ImGui::GetStyle().FramePadding.x)) {
                        Action_Cancel();
                    }

                    ImGui::EndPopup();
                }
            }
        }

        return res;  // quit if true, else continue on the next frame
    }

    return true;  // quit the action
}

///////////////////////////////////////////////////////
//// ACTIONS //////////////////////////////////////////
///////////////////////////////////////////////////////

void MainFrontend::Action_Menu_NewProject() {
    /*
    new project :
    -	unsaved :
        -	add action : show unsaved dialog
        -	add action : open dialog for new project file name
    -	saved :
        -	add action : open dialog for new project file name
    */
    m_ActionSystem.Clear();
    Action_OpenUnSavedDialog_IfNeeded();
    m_ActionSystem.Add([this]() {
        CloseUnSavedDialog();
        IGFD::FileDialogConfig config;
        config.countSelectionMax = 1;
        config.flags = ImGuiFileDialogFlags_Modal;
        ImGuiFileDialog::Instance()->OpenDialog("NewProjectDlg", "New Project File", PROJECT_EXT, config);
        return true;
    });
    m_ActionSystem.Add([this]() { return Display_NewProjectDialog(); });
}

void MainFrontend::Action_Menu_OpenProject() {
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
    m_ActionSystem.Add([this]() {
        CloseUnSavedDialog();
        IGFD::FileDialogConfig config;
        config.countSelectionMax = 1;
        config.flags = ImGuiFileDialogFlags_Modal;
        ImGuiFileDialog::Instance()->OpenDialog("OpenProjectDlg", "Open Project File", PROJECT_EXT, config);
        return true;
    });
    m_ActionSystem.Add([this]() { return Display_OpenProjectDialog(); });
}

void MainFrontend::Action_Menu_ReOpenProject() {
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
    m_ActionSystem.Add([]() {
        MainBackend::Instance()->NeedToLoadProject(ProjectFile::Instance()->GetProjectFilepathName());
        return true;
    });
}

void MainFrontend::Action_Menu_SaveProject() {
    /*
    save project :
    -	never saved :
        -	add action : save as project
    -	saved in a file beofre :
        -	add action : save project
    */
    m_ActionSystem.Clear();
    m_ActionSystem.Add([this]() {
        if (!MainBackend::Instance()->SaveProject()) {
            CloseUnSavedDialog();
            IGFD::FileDialogConfig config;
            config.countSelectionMax = 1;
            config.flags = ImGuiFileDialogFlags_Modal;
            ImGuiFileDialog::Instance()->OpenDialog("SaveProjectDlg", "Save Project File", PROJECT_EXT, config);
        }
        return true;
    });
    m_ActionSystem.Add([this]() { return Display_SaveProjectDialog(); });
}

void MainFrontend::Action_Menu_SaveAsProject() {
    /*
    save as project :
    -	add action : save as project
    */
    m_ActionSystem.Clear();
    m_ActionSystem.Add([this]() {
        CloseUnSavedDialog();
        IGFD::FileDialogConfig config;
        config.countSelectionMax = 1;
        config.flags = ImGuiFileDialogFlags_ConfirmOverwrite | ImGuiFileDialogFlags_Modal;
        ImGuiFileDialog::Instance()->OpenDialog("SaveProjectDlg", "Save Project File", PROJECT_EXT, config);
        return true;
    });
    m_ActionSystem.Add([this]() { return Display_SaveProjectDialog(); });
}

void MainFrontend::Action_Menu_CloseProject() {
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
    m_ActionSystem.Add([]() {
        MainBackend::Instance()->NeedToCloseProject();
        return true;
    });
}

void MainFrontend::Action_Window_CloseApp() {
    if (MainBackend::Instance()->IsNeedToCloseApp())
        return;  // block next call to close app when running
    /*
    Close app :
    -	unsaved :
        -	add action : show unsaved dialog
        -	add action : Close app
    -	saved :
        -	add action : Close app
    */

    m_ActionSystem.Clear();
    Action_OpenUnSavedDialog_IfNeeded();
    m_ActionSystem.Add([]() {
        MainBackend::Instance()->CloseApp();
        return true;
    });
}

void MainFrontend::Action_OpenUnSavedDialog_IfNeeded() {
    if (ProjectFile::Instance()->IsProjectLoaded() && ProjectFile::Instance()->IsThereAnyProjectChanges()) {
        OpenUnSavedDialog();
        m_ActionSystem.Add([this]() { return ShowUnSavedDialog(); });
    }
}

void MainFrontend::Action_Cancel() {
    /*
    -	cancel :
        -	clear actions
    */
    CloseUnSavedDialog();
    m_ActionSystem.Clear();
    MainBackend::Instance()->NeedToCloseApp(false);
}

bool MainFrontend::Action_UnSavedDialog_SaveProject() {
    bool res = MainBackend::Instance()->SaveProject();
    if (!res) {
        m_ActionSystem.Insert([this]() { return Display_SaveProjectDialog(); });
        m_ActionSystem.Insert([this]() {
            CloseUnSavedDialog();
            IGFD::FileDialogConfig config;
            config.countSelectionMax = 1;
            config.flags = ImGuiFileDialogFlags_ConfirmOverwrite | ImGuiFileDialogFlags_Modal;
            config.path = ".";
            ImGuiFileDialog::Instance()->OpenDialog("SaveProjectDlg", "Save Project File", PROJECT_EXT, config);
            return true;
        });
    }
    return res;
}

void MainFrontend::Action_UnSavedDialog_SaveAsProject() {
    m_ActionSystem.Insert([this]() { return Display_SaveProjectDialog(); });
    m_ActionSystem.Insert([this]() {
        CloseUnSavedDialog();
        IGFD::FileDialogConfig config;
        config.countSelectionMax = 1;
        config.flags = ImGuiFileDialogFlags_ConfirmOverwrite | ImGuiFileDialogFlags_Modal;
        config.path = ".";
        ImGuiFileDialog::Instance()->OpenDialog("SaveProjectDlg", "Save Project File", PROJECT_EXT, config);
        return true;
    });
}

void MainFrontend::Action_UnSavedDialog_Cancel() {
    Action_Cancel();
}

///////////////////////////////////////////////////////
//// DIALOG FUNCS /////////////////////////////////////
///////////////////////////////////////////////////////

bool MainFrontend::Display_NewProjectDialog() {
    // need to return false to continue to be displayed next frame

    ImVec2 min = m_DisplaySize * 0.5f;
    ImVec2 max = m_DisplaySize;

    if (ImGuiFileDialog::Instance()->Display("NewProjectDlg", ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking, min, max)) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            CloseUnSavedDialog();
            auto file = ImGuiFileDialog::Instance()->GetFilePathName();
            MainBackend::Instance()->NeedToNewProject(file);
        } else  // cancel
        {
            Action_Cancel();  // we interrupts all actions
        }

        ImGuiFileDialog::Instance()->Close();

        return true;
    }

    return false;
}

bool MainFrontend::Display_OpenProjectDialog() {
    // need to return false to continue to be displayed next frame

    ImVec2 min = m_DisplaySize * 0.5f;
    ImVec2 max = m_DisplaySize;

    if (ImGuiFileDialog::Instance()->Display("OpenProjectDlg", ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking, min, max)) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            CloseUnSavedDialog();
            MainBackend::Instance()->NeedToLoadProject(ImGuiFileDialog::Instance()->GetFilePathName());
        } else  // cancel
        {
            Action_Cancel();  // we interrupts all actions
        }

        ImGuiFileDialog::Instance()->Close();

        return true;
    }

    return false;
}

bool MainFrontend::Display_SaveProjectDialog() {
    // need to return false to continue to be displayed next frame

    ImVec2 min = m_DisplaySize * 0.5f;
    ImVec2 max = m_DisplaySize;

    if (ImGuiFileDialog::Instance()->Display("SaveProjectDlg", ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking, min, max)) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            CloseUnSavedDialog();
            MainBackend::Instance()->SaveAsProject(ImGuiFileDialog::Instance()->GetFilePathName());
        } else  // cancel
        {
            Action_Cancel();  // we interrupts all actions
        }

        ImGuiFileDialog::Instance()->Close();

        return true;
    }

    return false;
}

///////////////////////////////////////////////////////
//// APP CLOSING //////////////////////////////////////
///////////////////////////////////////////////////////

void MainFrontend::IWantToCloseTheApp() {
    Action_Window_CloseApp();
}

///////////////////////////////////////////////////////
//// DROP /////////////////////////////////////////////
///////////////////////////////////////////////////////

void MainFrontend::JustDropFiles(int count, const char** paths) {
    assert(0);

    std::map<std::string, std::string> dicoFont;
    std::string prj;

    for (int i = 0; i < count; ++i) {
        // file
        auto f = std::string(paths[i]);

        // lower case
        auto f_opt = f;
        for (auto& c : f_opt)
            c = (char)std::tolower((int)c);

        // well known extention
        if (f_opt.find(".ttf") != std::string::npos     // truetype (.ttf)
            || f_opt.find(".otf") != std::string::npos  // opentype (.otf)
            //||	f_opt.find(".ttc") != std::string::npos		// ttf/otf collection for futur (.ttc)
        ) {
            dicoFont[f] = f;
        }
        if (f_opt.find(PROJECT_EXT) != std::string::npos) {
            prj = f;
        }
    }

    // priority to project file
    if (!prj.empty()) {
        MainBackend::Instance()->NeedToLoadProject(prj);
    }
}

//////////////////////////////////////////////////////////////////////////////////
//// PRIVATE /////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////

bool MainFrontend::m_build() {
    // toolbar
    /*static ImFontConfig icons_config3;
    icons_config3.MergeMode = false;
    icons_config3.PixelSnapH = true;
    static const ImWchar icons_ranges3[] = {ICON_MIN_SDFMT, ICON_MAX_SDFMT, 0};
    const float& font_size = 20.0f / font_scale_ratio;
    m_ToolbarFontPtr =
        ImGui::GetIO().Fonts->AddFontFromMemoryCompressedBase85TTF(FONT_ICON_BUFFER_NAME_SDFMT, font_size, &icons_config3, icons_ranges3);
    if (m_ToolbarFontPtr != nullptr) {
        m_ToolbarFontPtr->Scale = font_scale_ratio;
        return true;
    }*/
    return true;
}

///////////////////////////////////////////////////////
//// CONFIGURATION ////////////////////////////////////
///////////////////////////////////////////////////////

ez::xml::Nodes MainFrontend::getXmlNodes(const std::string& /*vUserDatas*/) {
    ez::xml::Node node("root");
    node.addChilds(ImGuiThemeHelper::Instance()->getXmlNodes());
    node.addChilds(LayoutManager::Instance()->getXmlNodes("app"));
    node.addChild("places").setContent(ImGuiFileDialog::Instance()->SerializePlaces());
    node.addChild("showaboutdialog").setContent(m_ShowAboutDialog ? " true " : " false ");
    node.addChild("showimgui").setContent(m_ShowImGui ? "true" : "false");
    node.addChild("showmetric").setContent(m_ShowMetric ? "true" : "false");
    return node.getChildren();
}

bool MainFrontend::setFromXmlNodes(const ez::xml::Node& vNode, const ez::xml::Node& vParent, const std::string& vUserDatas) {
    UNUSED(vUserDatas);
    const auto& strName = vNode.getName();
    const auto& strValue = vNode.getContent();
    // const auto& strParentName = vParent.getName();

    ImGuiThemeHelper::Instance()->setFromXmlNodes(vNode, vParent, "app");
    LayoutManager::Instance()->setFromXmlNodes(vNode, vParent, "app");

    if (strName == "places") {
        ImGuiFileDialog::Instance()->DeserializePlaces(strValue);
    } else if (strName == "showaboutdialog") {
        m_ShowAboutDialog = ez::ivariant(strValue).GetB();
    } else if (strName == "showimgui") {
        m_ShowImGui = ez::ivariant(strValue).GetB();
    } else if (strName == "showmetric") {
        m_ShowMetric = ez::ivariant(strValue).GetB();
    }

    return true;
}

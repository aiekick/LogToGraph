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

#include <headers/DatasDef.h>

#include "MainFrontend.h"

#include <backend/MainBackend.h>

#include <project/ProjectFile.h>

#include <systems/PluginManager.h>

#include <panes/misc/ConsolePane.h>
#include <panes/misc/ProfilerPane.h>
#include <panes/misc/CodePane.h>
#include <panes/log/LogPane.h>
#include <panes/misc/CodePane.h>
#include <panes/misc/ToolPane.h>
#include <panes/graph/GraphPane.h>
#include <panes/misc/ConsolePane.h>
#include <panes/graph/GraphGroupPane.h>
#include <panes/signals/SignalsHoveredDiff.h>
#include <panes/signals/SignalsHoveredList.h>
#include <panes/signals/SignalsHoveredMap.h>
#include <panes/log/LogPaneSecondView.h>
#include <panes/graph/GraphListPane.h>
#include <panes/graph/AnnotationPane.h>
#include <panes/debug/StackTreePane.h>
#include <panes/debug/ScopePane.h>
#include <panes/debug/CalltracePane.h>
#include <panes/debug/BreakpointsPane.h>
#include <panes/debug/WatcherPane.h>

#include <fonts/fontIcons.h>

#include <ezlibs/ezFile.hpp>

#include <settings/SettingsDialog.h>

#include <systems/TranslationHelper.h>

#include <headers/LogToGraphBuild.h>

// panes
#define DEBUG_PANE_ICON ICON_FONT_BUG
#define SCENE_PANE_ICON ICON_FONT_FORMAT_LIST_BULLETED_TYPE
#define TUNING_PANE_ICON ICON_FONT_TUNE
#define CONSOLE_PANE_ICON ICON_FONT_COMMENT_TEXT_MULTIPLE

// features
#define GRID_ICON ICON_FONT_GRID
#define MOUSE_ICON ICON_FONT_MOUSE
#define CAMERA_ICON ICON_FONT_CAMCORDER
#define GIZMO_ICON ICON_FONT_AXIS_ARROW

using namespace std::placeholders;

//////////////////////////////////////////////////////////////////////////////////
//// STATIC //////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////

bool MainFrontend::sCentralWindowHovered = false;

//////////////////////////////////////////////////////////////////////////////////
//// PUBLIC //////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////

// clang-format off
bool MainFrontend::init() {
    m_build_themes();

    ImLayout::ref().init(ICON_FONT_TABLET_DASHBOARD " Layouts", "Default Layout");

    ImLayout::ref().setPaneDisposalRatio("LEFT", 0.25f);
    ImLayout::ref().setPaneDisposalRatio("RIGHT", 0.25f);
    ImLayout::ref().setPaneDisposalRatio("BOTTOM", 0.25f);

    // misc
    ImLayout::ref().addPane(
        LayoutPaneInfos(CodePane::ref(), ICON_FONT_CODE_BRACES " Code")
            .setMenu(ICON_FONT_CODE_BRACES " Code", "Misc")
            .setDisposalCentral());
    ImLayout::ref().addPane(
        LayoutPaneInfos(ConsolePane::ref(), ICON_FONT_COMMENT_TEXT_MULTIPLE " Console")
            .setMenu(ICON_FONT_COMMENT_TEXT_MULTIPLE " Console", "Misc")
            .setDisposalSide("BOTTOM", 0.3f));
    ImLayout::ref().addPane(
        LayoutPaneInfos(ProfilerPane::ref(), ICON_FONT_CHART_DONUT_VARIANT " Profiler")
            .setMenu(ICON_FONT_CHART_DONUT_VARIANT " Profiler", "Misc")
            .setDisposalSide("BOTTOM", 0.3f));
    ImLayout::ref().addPane(
        LayoutPaneInfos(ToolPane::ref(), ICON_FONT_CUBE_SCAN " Tool")
            .setMenu(ICON_FONT_CUBE_SCAN " Tool", "Misc")
            .setDisposalSide("LEFT", 0.25f)
            .setDefaultOpened(true)
            .setDefaultFocused(true));

    // logs
    ImLayout::ref().addPane(
        LayoutPaneInfos(LogPane::ref(), ICON_FONT_FILE_DOCUMENT_BOX " Logs")
            .setMenu(ICON_FONT_FILE_DOCUMENT_BOX " Logs")
            .setDisposalSide("RIGHT", 0.25f)
            .setDefaultOpened(true));
    ImLayout::ref().addPane(
        LayoutPaneInfos(LogPaneSecondView::ref(), ICON_FONT_FILE_DOCUMENT_BOX " Logs 2nd")
            .setMenu(ICON_FONT_FILE_DOCUMENT_BOX " Logs 2nd")
            .setDisposalSide("RIGHT", 0.25f));

    // graph
    ImLayout::ref().addPane(
        LayoutPaneInfos(AnnotationPane::ref(), ICON_FONT_CARDS " Annotations").setMenu(ICON_FONT_CARDS " Annotations").setDisposalSide("RIGHT", 0.25f));
    ImLayout::ref().addPane(
        LayoutPaneInfos(GraphPane::ref(), ICON_FONT_CHART_LINE " Graphs")
            .setMenu(ICON_FONT_CHART_LINE " Graphs")
            .setDisposalCentral()
            .setDefaultOpened(true));
    ImLayout::ref().addPane(
        LayoutPaneInfos(GraphListPane::ref(), ICON_FONT_CHART_LINE " All Graph Signals")
            .setMenu(ICON_FONT_CHART_LINE " All Graph Signals")
            .setDisposalCentral());
    ImLayout::ref().addPane(
        LayoutPaneInfos(GraphGroupPane::ref(), ICON_FONT_BUFFER " Graph Groups")
            .setMenu(ICON_FONT_BUFFER " Graph Groups")
            .setDisposalSide("RIGHT", 0.25f)
            .setDefaultOpened(true));
    ImLayout::ref().addPane(
        LayoutPaneInfos(SignalsHoveredList::ref(), ICON_FONT_CACTUS " Signals Hovered List")
            .setMenu(ICON_FONT_CACTUS " Signals Hovered List")
            .setDisposalSide("RIGHT", 0.25f));
    ImLayout::ref().addPane(
        LayoutPaneInfos(SignalsHoveredDiff::ref(), ICON_FONT_VECTOR_DIFFERENCE " Signals Hovered Diff")
            .setMenu(ICON_FONT_VECTOR_DIFFERENCE " Signals Hovered Diff")
            .setDisposalSide("RIGHT", 0.25f));

    // debug
    ImLayout::ref().addPane(
        LayoutPaneInfos(BreakpointsPane::ref(), ICON_FONT_BUG " Breakpoints")
            .setMenu(ICON_FONT_BUG " Breakpoints", "Debug")
            .setDisposalSide("BOTTOM", 0.3f));
    ImLayout::ref().addPane(
        LayoutPaneInfos(CalltracePane::ref(), ICON_FONT_FORMAT_LIST_BULLETED " Call Trace")
            .setMenu(ICON_FONT_FORMAT_LIST_BULLETED " Call Trace", "Debug")
            .setDisposalSide("BOTTOM", 0.3f));
    ImLayout::ref().addPane(
        LayoutPaneInfos(StackTreePane::ref(), ICON_FONT_FILE_TREE " Stack Tree")
            .setMenu(ICON_FONT_FILE_TREE " Stack Tree", "Debug")
            .setDisposalSide("BOTTOM", 0.3f));
    ImLayout::ref().addPane(
        LayoutPaneInfos(ScopePane::ref(), ICON_FONT_CROSSHAIRS " Scope")
            .setMenu(ICON_FONT_CROSSHAIRS " Scope", "Debug")
            .setDisposalSide("BOTTOM", 0.3f));
    ImLayout::ref().addPane(
        LayoutPaneInfos(WatcherPane::ref(), ICON_FONT_EYE " Watcher")
            .setMenu(ICON_FONT_EYE " Watcher", "Debug")
            .setDisposalSide("BOTTOM", 0.3f));

    // InitPanes is done in m_InitPanes, because a specific order is needed

    return m_build();
}
// clang-format on

void MainFrontend::unit() {
    ImLayout::ref().unitPanes();
}

bool MainFrontend::isValid() const {
    return false;
}

bool MainFrontend::isThereAnError() const {
    return false;
}

void MainFrontend::Display(const uint32_t& vCurrentFrame, const ImVec2& vPos, const ImVec2& vSize) {
    ImGui::CustomStyle::ResetCustomId();
    const auto context_ptr = ImGui::GetCurrentContext();
    if (context_ptr != nullptr) {
        const auto& io = ImGui::GetIO();

        m_DisplayPos = vPos;
        m_DisplaySize = vSize;

        MainFrontend::sCentralWindowHovered = (ImGui::GetCurrentContext()->HoveredWindow == nullptr);

        // global Ctrl+S — save the project (works regardless of focus; the editor's own Ctrl+S
        // also fires when focused, both converge on a Save which is idempotent).
        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S, false)) {
            Action_Menu_SaveProject();
        }

        // m_drawLeftButtonBar();
        m_drawMainMenuBar();
        m_drawMainStatusBar();

        if (ImLayout::ref().beginDockSpace(ImGuiDockNodeFlags_PassthruCentralNode)) {
            /*if (MainBackend::ref().GetBackendDatasRef().canWeTuneGizmo) {
                const auto viewport = ImGui::GetMainViewport();
                ImGuizmo::SetDrawlist(ImGui::GetCurrentWindow()->DrawList);
                ImGuizmo::SetRect(viewport->Pos.x, viewport->Pos.y, viewport->Size.x, viewport->Size.y);
                ImRect rc(viewport->Pos.x, viewport->Pos.y, viewport->Size.x, viewport->Size.y);
                DrawOverlays(vCurrentFrame, rc, context_ptr, {});
            }*/
            ImLayout::ref().endDockSpace();
        }

        if (ImLayout::ref().drawPanes({})) {
            ProjectFile::ref()->SetProjectChange();
        }

        DrawDialogsAndPopups(vCurrentFrame, ImRect(ImVec2(0,0), m_DisplaySize), context_ptr, {});

        ImGuiThemeHelper::ref().Draw();
        ImLayout::ref().initAfterFirstDisplay(io.DisplaySize);
    }
}

bool MainFrontend::DrawDialogsAndPopups(const uint32_t& vCurrentFrame, const ImRect& vMaxRect, ImGuiContext* vContextPtr, void* vUserDatas) {
    m_ActionSystem.executeFirstConditionalAction();
    ImLayout::ref().drawDialogsAndPopups(vMaxRect, vUserDatas);
    if (m_ShowImGui) {
        ImGui::ShowDemoWindow(&m_ShowImGui);
    }
    if (m_ShowImPlot) {
        ImPlot::ShowDemoWindow(&m_ShowImPlot);
    }
    if (m_ShowMetric) {
        ImGui::ShowMetricsWindow(&m_ShowMetric);
    }
    SettingsDialog::ref().Draw();
    m_drawAboutDialog();
    m_DrawImportScriptDialog();
    return false;
}

void MainFrontend::m_drawAboutDialog() {
    if (m_ShowAboutDialog) {
        ImGui::OpenPopup("About");
        if (ImGui::BeginPopupModal("About", &m_ShowAboutDialog, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar)) {
            ImGui::BeginGroup();

            // texture is inverted, so we invert uv.y
            auto texID = (ImTextureID)(void*)(size_t)MainBackend::ref().getBigAppIconID();
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

            if (ProjectFile::ref()->IsProjectLoaded()) {
                ImGui::Separator();

                if (ImGui::MenuItem(ICON_FONT_FOLDER_OPEN " Re Open")) {
                    Action_Menu_ReOpenProject();
                }

                ImGui::Separator();

                if (ImGui::MenuItem(ICON_FONT_FLOPPY " Save", "Ctrl+S")) {
                    Action_Menu_SaveProject();
                }

                if (ImGui::MenuItem(ICON_FONT_FLOPPY " Save As")) {
                    Action_Menu_SaveAsProject();
                }

                ImGui::Separator();

                if (ImGui::MenuItem(ICON_FONT_CLOSE" Close")) {
                    Action_Menu_CloseProject();
                }

                ImGui::Separator();

                if (ImGui::MenuItem(ICON_FONT_FILE " Import script")) {
                    IGFD::FileDialogConfig config;
                    config.countSelectionMax = 1;
                    config.flags = ImGuiFileDialogFlags_Modal;
                    ImGuiFileDialog::ref().OpenDialog("ImportScriptDlg", "Import a Lua script", ".lua,.*", config);
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
        ImLayout::ref().drawMenu(io.DisplaySize);

        ImGui::Spacing();

        if (ImGui::BeginMenu(ICON_FONT_TUNE " Tools")) {
            if (ImGui::MenuItem(ICON_FONT_SETTINGS " Settings")) {
                SettingsDialog::ref().OpenDialog();
            }
            ImGui::Separator();
            if (ImGui::BeginMenu(ICON_FONT_PALETTE " Styles")) {
                ImGuiThemeHelper::ref().DrawMenu();

                ImGui::Separator();

                ImGui::MenuItem("Show ImGui", "", &m_ShowImGui);
                ImGui::MenuItem("Show ImGui Metric/Debug", "", &m_ShowMetric);
                ImGui::MenuItem("Show ImPlot", "", &m_ShowImPlot);

                ImGui::EndMenu();
            }

            ImGui::EndMenu();
        }

        if (ProjectFile::ref()->IsThereAnyProjectChanges()) {
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

        s_translation_menu_size = TranslationHelper::ref().DrawMenu();

        ImGui::EndMainMenuBar();
    }
}

void MainFrontend::m_drawMainStatusBar() {
    if (ImGui::BeginMainStatusBar()) {
        Messaging::ref().DrawStatusBar();

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

void MainFrontend::m_DrawImportScriptDialog() {
    const ImVec2 maxSize = m_DisplaySize;
    const ImVec2 minSize = maxSize * 0.5f;
    if (ImGuiFileDialog::ref().Display("ImportScriptDlg", ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking, minSize, maxSize)) {
        if (ImGuiFileDialog::ref().IsOk()) {
            const auto filePathName = ImGuiFileDialog::ref().GetFilePathName();
            const auto code = ez::file::loadFileToString(filePathName);
            CodePane::ref()->OpenScript(code);
            ProjectFile::ref()->SetProjectChange();
        }
        ImGuiFileDialog::ref().Close();
    }
}

///////////////////////////////////////////////////////
//// SAVE DIALOG WHEN UN SAVED CHANGES ////////////////
///////////////////////////////////////////////////////

void MainFrontend::OpenUnSavedDialog() {
    // force close dialog if any dialog is opened
    ImGuiFileDialog::ref().Close();

    m_SaveDialogIfRequired = true;
}
void MainFrontend::CloseUnSavedDialog() {
    m_SaveDialogIfRequired = false;
}

bool MainFrontend::ShowUnSavedDialog() {
    bool res = false;

    if (m_SaveDialogIfRequired) {
        if (ProjectFile::ref()->IsProjectLoaded()) {
            if (ProjectFile::ref()->IsThereAnyProjectChanges()) {
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
    m_ActionSystem.clear();
    Action_OpenUnSavedDialog_IfNeeded();
    m_ActionSystem.pushBackConditonalAction([this]() {
        CloseUnSavedDialog();
        IGFD::FileDialogConfig config;
        config.countSelectionMax = 1;
        config.flags = ImGuiFileDialogFlags_Modal;
        ImGuiFileDialog::ref().OpenDialog("NewProjectDlg", "New Project File", PROJECT_EXT, config);
        return true;
    });
    m_ActionSystem.pushBackConditonalAction([this]() { return Display_NewProjectDialog(); });
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
    m_ActionSystem.clear();
    Action_OpenUnSavedDialog_IfNeeded();
    m_ActionSystem.pushBackConditonalAction([this]() {
        CloseUnSavedDialog();
        IGFD::FileDialogConfig config;
        config.countSelectionMax = 1;
        config.flags = ImGuiFileDialogFlags_Modal;
        ImGuiFileDialog::ref().OpenDialog("OpenProjectDlg", "Open Project File", PROJECT_EXT, config);
        return true;
    });
    m_ActionSystem.pushBackConditonalAction([this]() { return Display_OpenProjectDialog(); });
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
    m_ActionSystem.clear();
    Action_OpenUnSavedDialog_IfNeeded();
    m_ActionSystem.pushBackConditonalAction([]() {
        MainBackend::ref().NeedToLoadProject(ProjectFile::ref()->GetProjectFilepathName());
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
    m_ActionSystem.clear();
    m_ActionSystem.pushBackConditonalAction([this]() {
        if (!MainBackend::ref().SaveProject()) {
            CloseUnSavedDialog();
            IGFD::FileDialogConfig config;
            config.countSelectionMax = 1;
            config.flags = ImGuiFileDialogFlags_Modal;
            ImGuiFileDialog::ref().OpenDialog("SaveProjectDlg", "Save Project File", PROJECT_EXT, config);
        }
        return true;
    });
    m_ActionSystem.pushBackConditonalAction([this]() { return Display_SaveProjectDialog(); });
}

void MainFrontend::Action_Menu_SaveAsProject() {
    /*
    save as project :
    -	add action : save as project
    */
    m_ActionSystem.clear();
    m_ActionSystem.pushBackConditonalAction([this]() {
        CloseUnSavedDialog();
        IGFD::FileDialogConfig config;
        config.countSelectionMax = 1;
        config.flags = ImGuiFileDialogFlags_ConfirmOverwrite | ImGuiFileDialogFlags_Modal;
        ImGuiFileDialog::ref().OpenDialog("SaveProjectDlg", "Save Project File", PROJECT_EXT, config);
        return true;
    });
    m_ActionSystem.pushBackConditonalAction([this]() { return Display_SaveProjectDialog(); });
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
    m_ActionSystem.clear();
    Action_OpenUnSavedDialog_IfNeeded();
    m_ActionSystem.pushBackConditonalAction([]() {
        MainBackend::ref().NeedToCloseProject();
        return true;
    });
}

void MainFrontend::Action_Window_CloseApp() {
    if (MainBackend::ref().IsNeedToCloseApp())
        return;  // block next call to close app when running
    /*
    Close app :
    -	unsaved :
        -	add action : show unsaved dialog
        -	add action : Close app
    -	saved :
        -	add action : Close app
    */

    m_ActionSystem.clear();
    Action_OpenUnSavedDialog_IfNeeded();
    m_ActionSystem.pushBackConditonalAction([]() {
        MainBackend::ref().CloseApp();
        return true;
    });
}

void MainFrontend::Action_OpenUnSavedDialog_IfNeeded() {
    if (ProjectFile::ref()->IsProjectLoaded() && ProjectFile::ref()->IsThereAnyProjectChanges()) {
        OpenUnSavedDialog();
        m_ActionSystem.pushBackConditonalAction([this]() { return ShowUnSavedDialog(); });
    }
}

void MainFrontend::Action_Cancel() {
    /*
    -	cancel :
        -	clear actions
    */
    CloseUnSavedDialog();
    m_ActionSystem.clear();
    MainBackend::ref().NeedToCloseApp(false);
}

bool MainFrontend::Action_UnSavedDialog_SaveProject() {
    bool res = MainBackend::ref().SaveProject();
    if (!res) {
        m_ActionSystem.pushFrontConditonalAction([this]() { return Display_SaveProjectDialog(); });
        m_ActionSystem.pushFrontConditonalAction([this]() {
            CloseUnSavedDialog();
            IGFD::FileDialogConfig config;
            config.countSelectionMax = 1;
            config.flags = ImGuiFileDialogFlags_ConfirmOverwrite | ImGuiFileDialogFlags_Modal;
            config.path = ".";
            ImGuiFileDialog::ref().OpenDialog("SaveProjectDlg", "Save Project File", PROJECT_EXT, config);
            return true;
        });
    }
    return res;
}

void MainFrontend::Action_UnSavedDialog_SaveAsProject() {
    m_ActionSystem.pushFrontConditonalAction([this]() { return Display_SaveProjectDialog(); });
    m_ActionSystem.pushFrontConditonalAction([this]() {
        CloseUnSavedDialog();
        IGFD::FileDialogConfig config;
        config.countSelectionMax = 1;
        config.flags = ImGuiFileDialogFlags_ConfirmOverwrite | ImGuiFileDialogFlags_Modal;
        config.path = ".";
        ImGuiFileDialog::ref().OpenDialog("SaveProjectDlg", "Save Project File", PROJECT_EXT, config);
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

    if (ImGuiFileDialog::ref().Display("NewProjectDlg", ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking, min, max)) {
        if (ImGuiFileDialog::ref().IsOk()) {
            CloseUnSavedDialog();
            auto file = ImGuiFileDialog::ref().GetFilePathName();
            MainBackend::ref().NeedToNewProject(file);
        } else  // cancel
        {
            Action_Cancel();  // we interrupts all actions
        }

        ImGuiFileDialog::ref().Close();

        return true;
    }

    return false;
}

bool MainFrontend::Display_OpenProjectDialog() {
    // need to return false to continue to be displayed next frame

    ImVec2 min = m_DisplaySize * 0.5f;
    ImVec2 max = m_DisplaySize;

    if (ImGuiFileDialog::ref().Display("OpenProjectDlg", ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking, min, max)) {
        if (ImGuiFileDialog::ref().IsOk()) {
            CloseUnSavedDialog();
            MainBackend::ref().NeedToLoadProject(ImGuiFileDialog::ref().GetFilePathName());
        } else  // cancel
        {
            Action_Cancel();  // we interrupts all actions
        }

        ImGuiFileDialog::ref().Close();

        return true;
    }

    return false;
}

bool MainFrontend::Display_SaveProjectDialog() {
    // need to return false to continue to be displayed next frame

    ImVec2 min = m_DisplaySize * 0.5f;
    ImVec2 max = m_DisplaySize;

    if (ImGuiFileDialog::ref().Display("SaveProjectDlg", ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking, min, max)) {
        if (ImGuiFileDialog::ref().IsOk()) {
            CloseUnSavedDialog();
            MainBackend::ref().SaveAsProject(ImGuiFileDialog::ref().GetFilePathName());
        } else  // cancel
        {
            Action_Cancel();  // we interrupts all actions
        }

        ImGuiFileDialog::ref().Close();

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
    EZ_TOOLS_DEBUG_BREAK;
    /*
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
        MainBackend::ref().NeedToLoadProject(prj);
    }
    */
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
    node.addChilds(ImGuiThemeHelper::ref().getXmlNodes());
    node.addChilds(ImLayout::ref().getXmlNodes("app"));
    node.addChild("places").setContent(ImGuiFileDialog::ref().SerializePlaces());
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

    ImGuiThemeHelper::ref().setFromXmlNodes(vNode, vParent, "app");
    ImLayout::ref().setFromXmlNodes(vNode, vParent, "app");

    if (strName == "places") {
        ImGuiFileDialog::ref().DeserializePlaces(strValue);
    } else if (strName == "showaboutdialog") {
        m_ShowAboutDialog = ez::ivariant(strValue).GetB();
    } else if (strName == "showimgui") {
        m_ShowImGui = ez::ivariant(strValue).GetB();
    } else if (strName == "showmetric") {
        m_ShowMetric = ez::ivariant(strValue).GetB();
    }

    return true;
}

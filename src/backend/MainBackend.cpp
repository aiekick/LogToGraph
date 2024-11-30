#include "MainBackend.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <headers/LogToGraphBuild.h>

#define IMGUI_IMPL_API
#include <3rdparty/imgui_docking/backends/imgui_impl_opengl3.h>
#include <3rdparty/imgui_docking/backends/imgui_impl_glfw.h>

#include <ezlibs/ezFile.hpp>

#include <models/script/ScriptingEngine.h>

#include <cstdio>     // printf, fprintf
#include <chrono>     // timer
#include <cstdlib>    // abort
#include <fstream>    // std::ifstream
#include <iostream>   // std::cout
#include <algorithm>  // std::min, std::max
#include <stdexcept>  // std::exception

#include <backend/MainBackend.h>
#include <systems/PluginManager.h>
#include <project/ProjectFile.h>

#include <LayoutManager.h>

#include <ImGuiPack.h>
#include <iagp/iagp.h>

#include <frontend/MainFrontend.h>

#include <panes/ConsolePane.h>

#include <systems/SettingsDialog.h>

// we include the cpp just for embedded fonts
#include <res/fontIcons.cpp>
#include <res/Roboto_Medium.cpp>

#include <filesystem>
namespace fs = std::filesystem;

#define INITIAL_WIDTH 1700
#define INITIAL_HEIGHT 700

//////////////////////////////////////////////////////////////////////////////////
//// STATIC //////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////

static void glfw_error_callback(int error, const char* description) {
    LogVarError("glfw error %i : %s", error, description);
}

static void glfw_window_close_callback(GLFWwindow* window) {
    glfwSetWindowShouldClose(window, GLFW_FALSE);  // block app closing
    MainFrontend::Instance()->Action_Window_CloseApp();
}

//////////////////////////////////////////////////////////////////////////////////
//// PUBLIC //////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////

MainBackend::~MainBackend() = default;

void MainBackend::run(const std::string& vAppPath) {
    if (init(vAppPath)) {
        m_MainLoop();
        unit(vAppPath);
    }
}

// todo : to refactor ! i dont like that
bool MainBackend::init(const std::string& vAppPath) {
#ifdef _DEBUG
    SetConsoleVisibility(true);
#else
    SetConsoleVisibility(true);
#endif
    if (m_InitWindow() && m_InitImGui()) {
        m_InitPlugins(vAppPath);
        m_InitModels();
        m_InitSystems();
        m_InitPanes();
        m_InitSettings();
        LoadConfigFile(fs::path(vAppPath).append("config.xml").string(), "app");
        return true;
    }
    return false;
}

// todo : to refactor ! i dont like that
void MainBackend::unit(const std::string& vAppPath) {
    SaveConfigFile(fs::path(vAppPath).append("config.xml").string(), "app", "config");
    m_UnitSystems();
    m_UnitModels();
    m_UnitImGui();     // before plugins since containing weak ptr to plugins
    m_UnitSettings();  // after gui but before plugins since containing weak ptr to plugins
    m_UnitPlugins();
    m_UnitWindow();
}

bool MainBackend::isThereAnError() const {
    return false;
}

void MainBackend::NeedToNewProject(const std::string& vFilePathName) {
    m_NeedToNewProject = true;
    m_ProjectFileToLoad = vFilePathName;
}

void MainBackend::NeedToLoadProject(const std::string& vFilePathName) {
    m_NeedToLoadProject = true;
    m_ProjectFileToLoad = vFilePathName;
}

void MainBackend::NeedToCloseProject() {
    m_NeedToCloseProject = true;
}

bool MainBackend::SaveProject() {
    return ProjectFile::Instance()->Save();
}

void MainBackend::SaveAsProject(const std::string& vFilePathName) {
    ProjectFile::Instance()->SaveAs(vFilePathName);
}

// actions to do after rendering
void MainBackend::PostRenderingActions() {
    if (m_NeedToNewProject) {
        ProjectFile::Instance()->Clear();
        ProjectFile::Instance()->ClearDatas();
        ProjectFile::Instance()->New(m_ProjectFileToLoad);
        m_ProjectFileToLoad.clear();
        m_NeedToNewProject = false;
    }

    if (m_NeedToLoadProject) {
        if (!m_ProjectFileToLoad.empty()) {
            if (ProjectFile::Instance()->LoadAs(m_ProjectFileToLoad)) {
                setAppTitle(m_ProjectFileToLoad);
                ProjectFile::Instance()->SetProjectChange(false);
            } else {
                LogVarError("Failed to load project %s", m_ProjectFileToLoad.c_str());
            }
        }

        m_ProjectFileToLoad.clear();
        m_NeedToLoadProject = false;
    }

    if (m_NeedToCloseProject) {
        ProjectFile::Instance()->Clear();
        ProjectFile::Instance()->ClearDatas();
        m_NeedToCloseProject = false;
    }
}

bool MainBackend::IsNeedToCloseApp() {
    return m_NeedToCloseApp;
}

void MainBackend::NeedToCloseApp(const bool& vFlag) {
    m_NeedToCloseApp = vFlag;
}

void MainBackend::CloseApp() {
    // will escape the main loop
    glfwSetWindowShouldClose(m_MainWindowPtr, 1);
}

void MainBackend::setAppTitle(const std::string& vFilePathName) {
    auto ps = ez::file::parsePathFileName(vFilePathName);
    if (ps.isOk) {
        char bufTitle[1024];
        snprintf(bufTitle, 1023, "%s Beta %s - Project : %s.lum", LogToGraph_Prefix, LogToGraph_BuildId, ps.name.c_str());
        glfwSetWindowTitle(m_MainWindowPtr, bufTitle);
    } else {
        char bufTitle[1024];
        snprintf(bufTitle, 1023, "%s Beta %s", LogToGraph_Prefix, LogToGraph_BuildId);
        glfwSetWindowTitle(m_MainWindowPtr, bufTitle);
    }
}

ez::dvec2 MainBackend::GetMousePos() {
    ez::dvec2 mp;
    glfwGetCursorPos(m_MainWindowPtr, &mp.x, &mp.y);
    return mp;
}

int MainBackend::GetMouseButton(int vButton) {
    return glfwGetMouseButton(m_MainWindowPtr, vButton);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
//// CONSOLE ///////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////

void MainBackend::SetConsoleVisibility(const bool& vFlag) {
    m_ConsoleVisiblity = vFlag;

    if (m_ConsoleVisiblity) {
        // on cache la console
        // on l'affichera au besoin comme blender fait
#ifdef WIN32
        ShowWindow(GetConsoleWindow(), SW_SHOW);
#endif
    } else {
        // on cache la console
        // on l'affichera au besoin comme blender fait
#ifdef WIN32
        ShowWindow(GetConsoleWindow(), SW_HIDE);
#endif
    }
}

void MainBackend::SwitchConsoleVisibility() {
    m_ConsoleVisiblity = !m_ConsoleVisiblity;
    SetConsoleVisibility(m_ConsoleVisiblity);
}

bool MainBackend::GetConsoleVisibility() {
    return m_ConsoleVisiblity;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
//// RENDER ////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////

void MainBackend::m_RenderOffScreen() {
    // m_DisplaySizeQuadRendererPtr->SetImageInfos(m_MergerRendererPtr->GetBackDescriptorImageInfo(0U));
}

void MainBackend::m_MainLoop() {
    int display_w, display_h;
    ImVec2 pos, size;
    while (!glfwWindowShouldClose(m_MainWindowPtr)) {
        {
#ifndef _DEBUG
            if (!ScriptingEngine::Instance()->IsJoinable()) {  // for not blocking threading progress bar animation
                glfwWaitEventsTimeout(1.0);
            }
#endif
            IAGPNewFrame("GPU Frame", "GPU Frame");  // a main Zone is always needed

            ProjectFile::Instance()->NewFrame();

            // maintain active, prevent user change via imgui dialog
            ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;    // Enable Docking
            ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;  // Disable Viewport

            glfwPollEvents();

            glfwGetFramebufferSize(m_MainWindowPtr, &display_w, &display_h);

            m_Update();  // to do absolutly before imgui rendering

            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            ImGuiViewport* viewport = ImGui::GetMainViewport();
            if (viewport) {
                pos = viewport->WorkPos;
                size = viewport->WorkSize;
            }

            MainFrontend::Instance()->Display(m_CurrentFrame, pos, size);

            ScriptingEngine::Instance()->FinishIfRequired();

            ImGui::Render();

            glViewport(0, 0, display_w, display_h);
            glClear(GL_COLOR_BUFFER_BIT);
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            auto* backup_current_context = glfwGetCurrentContext();

            // Update and Render additional Platform Windows
            // (Platform functions may change the current OpenGL context, so we save/restore it to make it easier to paste
            // this code elsewhere.
            //  For this specific demo app we could also call glfwMakeContextCurrent(window) directly)
            if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
                ImGui::UpdatePlatformWindows();
                ImGui::RenderPlatformWindowsDefault();
            }
            glfwMakeContextCurrent(backup_current_context);

#ifdef USE_THUMBNAILS
            ImGuiFileDialog::Instance()->ManageGPUThumbnails();
#endif

            glfwSwapBuffers(m_MainWindowPtr);

            // mainframe post actions
            PostRenderingActions();

            ++m_CurrentFrame;
        }
        IAGPCollect;

        // will pause the view until we move the mouse or press keys
        // glfwWaitEvents();
    }
}

void MainBackend::m_Update() {}

void MainBackend::m_IncFrame() {
    ++m_CurrentFrame;
}

///////////////////////////////////////////////////////
//// CONFIGURATION ////////////////////////////////////
///////////////////////////////////////////////////////

ez::xml::Nodes MainBackend::getXmlNodes(const std::string& vUserDatas) {
    ez::xml::Node node("root");
    node.addChilds(MainFrontend::Instance()->getXmlNodes(vUserDatas));
    node.addChilds(SettingsDialog::Instance()->getXmlNodes(vUserDatas));
    node.addChild("project").setContent(ProjectFile::Instance()->GetProjectFilepathName());
    return node.getChildren();
}

bool MainBackend::setFromXmlNodes(const ez::xml::Node& vNode, const ez::xml::Node& vParent, const std::string& vUserDatas) {
    UNUSED(vUserDatas);
    const auto& strName = vNode.getName();
    const auto& strValue = vNode.getContent();
    // const auto& strParentName = vParent.getName();

    MainFrontend::Instance()->setFromXmlNodes(vNode, vParent, vUserDatas);
    SettingsDialog::Instance()->setFromXmlNodes(vNode, vParent, vUserDatas);

    if (strName == "project") {
        NeedToLoadProject(strValue);
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////////////
//// PRIVATE /////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////

bool MainBackend::m_InitWindow() {
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return false;

    // GL 3.0 + GLSL 130
    m_glslVersion = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_SAMPLES, 4);

    // Create window with graphics context
    m_MainWindowPtr = glfwCreateWindow(INITIAL_WIDTH, INITIAL_HEIGHT, LogToGraph_Prefix, nullptr, nullptr);
    if (m_MainWindowPtr == nullptr) {
        std::cout << "Fail to create the window" << std::endl;
        return false;
    }
    glfwMakeContextCurrent(m_MainWindowPtr);
    glfwSwapInterval(1);  // Enable vsync

    if (gladLoadGL() == 0) {
        fprintf(stderr, "Failed to initialize OpenGL loader!\n");
        return false;
    }

    glfwSetWindowCloseCallback(m_MainWindowPtr, glfw_window_close_callback);

    return true;
}

void MainBackend::m_UnitWindow() {
    glfwDestroyWindow(m_MainWindowPtr);
    glfwTerminate();
}

bool MainBackend::m_InitImGui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;    // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;  // Enable Viewport
    io.FontAllowUserScaling = true;                      // activate zoom feature with ctrl + mousewheel
#ifdef USE_DECORATIONS_FOR_RESIZE_CHILD_WINDOWS
    io.ConfigViewportsNoDecoration = false;  // toujours mettre une frame aux fenetres enfants
#endif

    // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    // fonts
    {
        {  // main font
            if (ImGui::GetIO().Fonts->AddFontFromMemoryCompressedBase85TTF(FONT_ICON_BUFFER_NAME_RM, 15.0f) == nullptr) {
                assert(0);  // failed to load font
            }
        }
        {  // icon font
            static const ImWchar icons_ranges[] = {ICON_MIN_FONT, ICON_MAX_FONT, 0};
            ImFontConfig icons_config;
            icons_config.MergeMode = true;
            icons_config.PixelSnapH = true;
            if (ImGui::GetIO().Fonts->AddFontFromMemoryCompressedBase85TTF(FONT_ICON_BUFFER_NAME, 15.0f, &icons_config, icons_ranges) == nullptr) {
                assert(0);  // failed to load font
            }
        }
    }

    // Setup Platform/Renderer bindings
    if (ImGui_ImplGlfw_InitForOpenGL(m_MainWindowPtr, true) &&  //
        ImGui_ImplOpenGL3_Init(m_glslVersion)) {
        // ui init
        if (MainFrontend::Instance()->init()) {
            iagp::InAppGpuProfiler::Instance()->Clear();
            return true;
        }
    }
    return false;
}

void MainBackend::m_InitPlugins(const std::string& vAppPath) {
    PluginManager::Instance()->loadPlugins(vAppPath);
    auto pluginPanes = PluginManager::Instance()->getPluginPanes();
    for (auto& pluginPane : pluginPanes) {
        if (!pluginPane.pane.expired()) {
            LayoutManager::Instance()->AddPane(  //
                pluginPane.pane,
                pluginPane.name,
                pluginPane.category,
                pluginPane.disposal,
                pluginPane.disposalRatio,
                pluginPane.openedDefault,
                pluginPane.focusedDefault);
            auto plugin_ptr = std::dynamic_pointer_cast<Ltg::PluginPane>(pluginPane.pane.lock());
            if (plugin_ptr != nullptr) {
                plugin_ptr->SetProjectInstance(ProjectFile::Instance());
            }
        }
    }
}

void MainBackend::m_InitModels() {
    ScriptingEngine::Instance()->Init();
}

void MainBackend::m_UnitModels() {
    ProjectFile::Instance()->Clear();
    ProjectFile::Instance()->ClearDatas();
    ScriptingEngine::Instance()->Unit();
}

void MainBackend::m_UnitPlugins() {
    PluginManager::Instance()->unloadPlugins();
}

void MainBackend::m_InitSystems() {
    m_SetEmbeddedIconApp("IDI_ICON1");
    // m_AppIconID = ExtractEmbeddedIcon("IDI_ICON1");
    m_BigAppIconID = m_ExtractEmbeddedImage("IDB_BMP1");
}

void MainBackend::m_UnitSystems() {
    if (m_AppIconID) {
        glDeleteTextures(GL_TEXTURE_2D, &m_AppIconID);
        m_AppIconID = 0U;
    }
    if (m_BigAppIconID) {
        glDeleteTextures(GL_TEXTURE_2D, &m_BigAppIconID);
        m_BigAppIconID = 0U;
    }
}

void MainBackend::m_InitPanes() {
    if (LayoutManager::Instance()->InitPanes()) {
        // a faire apres InitPanes() sinon ConsolePane::Instance()->paneFlag vaudra 0 et changeras apres InitPanes()
        Messaging::Instance()->sMessagePaneId = ConsolePane::Instance()->GetFlag();
    }
}

void MainBackend::m_UnitPanes() {}

void MainBackend::m_InitSettings() {
    SettingsDialog::Instance()->init();
}

void MainBackend::m_UnitSettings() {
    SettingsDialog::Instance()->unit();
}

void MainBackend::m_UnitImGui() {
    MainFrontend::Instance()->unit();
    iagp::InAppGpuProfiler::Instance()->Clear();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();

    ImPlot::DestroyContext();
    ImGui::DestroyContext();
}

void MainBackend::m_SetEmbeddedIconApp(const char* vEmbeddedIconID) {
#if WIN32
    auto icon_h_inst = LoadIconA(GetModuleHandle(NULL), vEmbeddedIconID);
    SetClassLongPtrA(glfwGetWin32Window(m_MainWindowPtr), GCLP_HICON, (LONG_PTR)icon_h_inst);
#else
    (void)vEmbeddedIconID;
#endif
}

GLuint MainBackend::m_ExtractEmbeddedIcon(const char* vEmbeddedIconID) {
#if WIN32
    // embedded icon to opengl texture

    auto icon_h_inst = LoadIconA(GetModuleHandle(NULL), vEmbeddedIconID);

    ICONINFO app_icon_info;
    if (GetIconInfo(icon_h_inst, &app_icon_info)) {
        HDC hdc = GetDC(0);

        BITMAPINFO MyBMInfo = {0};
        MyBMInfo.bmiHeader.biSize = sizeof(MyBMInfo.bmiHeader);
        if (GetDIBits(hdc, app_icon_info.hbmColor, 0, 0, NULL, &MyBMInfo, DIB_RGB_COLORS)) {
            uint8_t* bytes = new uint8_t[MyBMInfo.bmiHeader.biSizeImage];

            MyBMInfo.bmiHeader.biCompression = BI_RGB;
            if (GetDIBits(hdc, app_icon_info.hbmColor, 0, MyBMInfo.bmiHeader.biHeight, (LPVOID)bytes, &MyBMInfo, DIB_RGB_COLORS)) {
                uint8_t R, G, B;

                int index, i;

                // swap BGR to RGB
                for (i = 0; i < MyBMInfo.bmiHeader.biWidth * MyBMInfo.bmiHeader.biHeight; i++) {
                    index = i * 4;

                    B = bytes[index];
                    G = bytes[index + 1];
                    R = bytes[index + 2];

                    bytes[index] = R;
                    bytes[index + 1] = G;
                    bytes[index + 2] = B;
                }

                // create texture from loaded bmp image

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

GLuint MainBackend::m_ExtractEmbeddedImage(const char* vEmbeddedImageID) {
#if WIN32
    // embedded icon to opengl texture

    auto hBitmap = LoadImageA(GetModuleHandle(NULL), vEmbeddedImageID, IMAGE_BITMAP, 0, 0, LR_COPYFROMRESOURCE | LR_CREATEDIBSECTION);

    BITMAP bm;
    if (GetObjectA(hBitmap, sizeof(BITMAP), &bm)) {
        uint8_t* bytes = (uint8_t*)bm.bmBits;
        uint8_t R, G, B;

        int index, i;

        // swap BGR to RGB
        for (i = 0; i < bm.bmWidth * bm.bmHeight; i++) {
            index = i * 3;

            B = bytes[index];
            G = bytes[index + 1];
            R = bytes[index + 2];

            bytes[index] = R;
            bytes[index + 1] = G;
            bytes[index + 2] = B;
        }

        // create texture from loaded bmp image

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

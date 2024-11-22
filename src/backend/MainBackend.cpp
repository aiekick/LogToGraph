#include "MainBackend.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <headers/LogToGraphBuild.h>

#define IMGUI_IMPL_API
#include <3rdparty/imgui_docking/backends/imgui_impl_opengl3.h>
#include <3rdparty/imgui_docking/backends/imgui_impl_glfw.h>

#include <EzLibs/EzFile.hpp>

#include <cstdio>     // printf, fprintf
#include <chrono>     // timer
#include <cstdlib>    // abort
#include <fstream>    // std::ifstream
#include <iostream>   // std::cout
#include <algorithm>  // std::min, std::max
#include <stdexcept>  // std::exception

#include <Backend/MainBackend.h>
#include <systems/PluginManager.h>
#include <Project/ProjectFile.h>

#include <LayoutManager.h>

#include <ImGuiPack.h>
#include <iagp/iagp.h>

#include <Frontend/MainFrontend.h>

#include <panes/ConsolePane.h>

#include <Systems/SettingsDialog.h>

// we include the cpp just for embedded fonts
#include <Res/fontIcons.cpp>
#include <Res/Roboto_Medium.cpp>

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
    SetConsoleVisibility(false);
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
        snprintf(bufTitle, 1023, "Strocker Beta %s - Project : %s.lum", LogToGraph_BuildId, ps.name.c_str());
        glfwSetWindowTitle(m_MainWindowPtr, bufTitle);
    } else {
        char bufTitle[1024];
        snprintf(bufTitle, 1023, "Strocker Beta %s", LogToGraph_BuildId);
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
    m_GlslVersion = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_SAMPLES, 4);

    // Create window with graphics context
    m_MainWindowPtr = glfwCreateWindow(1280, 720, "MarketAnalyzer", nullptr, nullptr);
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
            static const ImWchar icons_ranges[] = {ICON_MIN_FONT, ICON_MIN_FONT, 0};
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
        ImGui_ImplOpenGL3_Init(m_GlslVersion)) {
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

void MainBackend::m_InitModels() {}

void MainBackend::m_UnitModels() {}

void MainBackend::m_UnitPlugins() {
    PluginManager::Instance()->unloadPlugins();
}

void MainBackend::m_InitSystems() {}

void MainBackend::m_UnitSystems() {}

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

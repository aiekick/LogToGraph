// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

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

#include <imgui/imgui.h>
#include "3rdparty/imgui/backends/imgui_impl_opengl3.h"
#include "3rdparty/imgui/backends/imgui_impl_glfw.h"
#include <stdio.h>

#include <ctools/FileHelper.h>
#include <Gui/MainFrame.h>
#include <Contrib/FontIcons/CustomFont.cpp>
#include <Contrib/FontIcons/CustomFont2.cpp>
#include <Contrib/FontIcons/CustomFontToolBar.cpp>
#include <Contrib/FontIcons/Roboto_Medium.cpp>
#include <glad/glad.h> 
#include <GLFW/glfw3.h>

#include <implot/implot.h>

#include <Engine/Lua/LuaEngine.h>

#define SHOW_CONSOLE

// [Win32] Our example includes a copy of glfw3.lib pre-compiled with VS2010 to maximize ease of testing and compatibility with old VS compilers.
// To link with VS2010-era libraries, VS2015+ requires linking with legacy_stdio_definitions.lib, which we do using this pragma.
// Your own project should not be affected, as you are likely to link with a newer binary of GLFW that is adequate for your version of Visual Studio.
#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

static void glfw_window_drop_callback(GLFWwindow* /*window*/, int count, const char** paths)
{
    MainFrame::Instance()->JustDropFiles(count, paths);
}

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

static void glfw_window_close_callback(GLFWwindow* window)
{
	glfwSetWindowShouldClose(window, GL_FALSE); // block app closing
	MainFrame::Instance()->IWantToCloseTheApp();
}

int main(int, char**argv)
{
	FileHelper::Instance()->SetAppPath(std::string(argv[0]));
    FileHelper::Instance()->SetCurDirectory(FileHelper::Instance()->GetAppPath());

    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

	// Decide GL+GLSL versions
#if APPLE
    // GL 3.2 + GLSL 150
    const char* glsl_version = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
#else
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
#endif

    glfwWindowHint(GLFW_SAMPLES, 4);
    
    // Create window with graphics context
    GLFWwindow* mainWindow = glfwCreateWindow(1280, 720, "LogToGraph", nullptr, nullptr);
    if (mainWindow == 0)
        return 1;
    glfwMakeContextCurrent(mainWindow);
    glfwSwapInterval(1); // Enable vsync
	glfwSetWindowCloseCallback(mainWindow, glfw_window_close_callback);
    glfwSetDropCallback(mainWindow, glfw_window_drop_callback);
    
    if (gladLoadGL() == 0)
    {
        fprintf(stderr, "Failed to initialize OpenGL loader!\n");
        return 1;
    }

#ifdef MSVC
	//#if defined(_DEBUG) && defined(SHOW_CONSOLE)
	//	ShowWindow(GetConsoleWindow(), SW_SHOW); // show 
    //#else
        ShowWindow(GetConsoleWindow(), SW_HIDE); // hide 
	//#endif
#endif

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
	
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Viewport
	io.FontAllowUserScaling = true; // activate zoom feature with ctrl + mousewheel

    // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    // Setup Platform/Renderer bindings
    ImGui_ImplGlfw_InitForOpenGL(mainWindow, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

	// load memory font file
    auto fonts_ptr = ImGui::GetIO().Fonts;
    fonts_ptr->AddFontFromMemoryCompressedBase85TTF(FONT_ICON_BUFFER_NAME_RM, 15.0f);
    static ImFontConfig icons_config; icons_config.MergeMode = true; icons_config.PixelSnapH = true;
    static const ImWchar icons_ranges[] = {ICON_MIN_NDP, ICON_MAX_NDP, 0};
    fonts_ptr->AddFontFromMemoryCompressedBase85TTF(FONT_ICON_BUFFER_NAME_NDP, 15.0f, &icons_config, icons_ranges);
    static const ImWchar icons_ranges2[] = {ICON_MIN_NDP2, ICON_MAX_NDP2, 0};
    fonts_ptr->AddFontFromMemoryCompressedBase85TTF(FONT_ICON_BUFFER_NAME_NDP2, 15.0f, &icons_config, icons_ranges2);
    static const ImWchar icons_ranges3[] = {ICON_MIN_NDPTB, ICON_MAX_NDPTB, 0};
    fonts_ptr->AddFontFromMemoryCompressedBase85TTF(FONT_ICON_BUFFER_NAME_NDPTB, 15.0f, &icons_config, icons_ranges3);

	MainFrame::Instance(mainWindow)->Init();

    // Main loop
	int display_w, display_h;
    ImVec2 pos, size;
	while (!glfwWindowShouldClose(mainWindow))
    {
        if (!LuaEngine::Instance()->IsJoinable()) // for not blocking threading progress bar animation
        {
            glfwWaitEventsTimeout(1.0);
        }
        glfwPollEvents();

        // maintain active, prevent user change via imgui dialog
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking

        glfwGetFramebufferSize(mainWindow, &display_w, &display_h);
        
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            ImGuiViewport* viewport = ImGui::GetMainViewport();
            if (viewport)
            {
                pos = viewport->WorkPos;
                size = viewport->WorkSize;
            }
        }
        else
        {
            pos = ImVec2(0, 0);
            size = ImVec2((float)display_w, (float)display_h);
        }

		MainFrame::Instance()->Display(pos, size);
        
        LuaEngine::Instance()->FinishIfRequired();

        ImGui::Render();

        glViewport(0, 0, display_w, display_h);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Update and Render additional Platform Windows
        // (Platform functions may change the current OpenGL context, so we save/restore it to make it easier to paste this code elsewhere.
        // For this specific demo app we could also call glfwMakeContextCurrent(window) directly)
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            GLFWwindow* backup_current_context = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backup_current_context);
        }

        glfwSwapBuffers(mainWindow);
    }

	MainFrame::Instance()->Unit();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();

	ImPlot::DestroyContext();
    ImGui::DestroyContext();

    glfwDestroyWindow(mainWindow);
    glfwTerminate();

    return 0;
}

#pragma once

#include <glad/glad.h>
#include <imguipack.h>
#include <ezlibs/ezTools.hpp>
#include <ezlibs/ezClass.hpp>
#include <ezlibs/ezSingleton.hpp>
#include <ezlibs/ezXmlConfig.hpp>

#include <string>
#include <memory>
#include <array>
#include <vector>
#include <functional>
#include <unordered_map>

struct GLFWwindow;
class MainBackend : public ez::xml::Config {
    DISABLE_CONSTRUCTORS(MainBackend)
    DISABLE_DESTRUCTORS(MainBackend)
    IMPLEMENT_SINGLETON(MainBackend)

private:
    GLFWwindow* m_MainWindowPtr = nullptr;
    const char* m_glslVersion = "";
    ImRect m_displayRect;

    // mouse
    ez::math::fvec4 m_MouseFrameSize;
    ez::math::fvec2 m_MousePos;
    ez::math::fvec2 m_LastNormalizedMousePos;
    ez::math::fvec2 m_NormalizedMousePos;

    uint32_t m_CurrentFrame = 0U;

    bool m_NeedToCloseApp = false;  // when app closing app is required

    bool m_NeedToNewProject = false;
    bool m_NeedToLoadProject = false;
    bool m_NeedToCloseProject = false;
    std::string m_ProjectFileToLoad;

    std::function<void(std::set<std::string>)> m_ChangeFunc;
    std::set<std::string> m_PathsToTrack;

    GLuint m_AppIconID = 0U;
    GLuint m_BigAppIconID = 0U;

public:  // getters
    ImRect GetDisplayRect() { return m_displayRect; }

public:
    void run(const std::string& vAppPath);

    bool init(const std::string& vAppPath);
    void unit(const std::string& vAppPath);

    bool isThereAnError() const;

    void NeedToNewProject(const std::string& vFilePathName);
    void NeedToLoadProject(const std::string& vFilePathName);
    void NeedToCloseProject();

    bool SaveProject();
    void SaveAsProject(const std::string& vFilePathName);

    void PostRenderingActions();

    bool IsNeedToCloseApp();
    void NeedToCloseApp(const bool vFlag = true);
    void CloseApp();

    void setAppTitle(const std::string& vFilePathName = {});

    GLuint getBigAppIconID() { return m_BigAppIconID; }

    ez::math::dvec2 GetMousePos();
    int GetMouseButton(int vButton);

public:  // configuration
    ez::xml::Nodes getXmlNodes(const std::string& vUserDatas = "") final;
    bool setFromXmlNodes(const ez::xml::Node& vNode, const ez::xml::Node& vParent, const std::string& vUserDatas) final;

private:
    void m_RenderOffScreen();

    bool m_InitWindow();
    bool m_InitImGui();
    void m_InitPlugins(const std::string& vAppPath);
    void m_InitModels();
    void m_InitSystems();
    void m_InitPanes();
    void m_InitSettings();

    void m_UnitWindow();
    void m_UnitModels();
    void m_UnitImGui();
    void m_UnitPlugins();
    void m_UnitSystems();
    void m_UnitPanes();
    void m_UnitSettings();

    void m_MainLoop();
    void m_Update();
    void m_IncFrame();

    void m_SetEmbeddedIconApp(const char* vEmbeddedIconID);
    GLuint m_ExtractEmbeddedIcon(const char* vEmbeddedIconID);
    GLuint m_ExtractEmbeddedImage(const char* vEmbeddedImageID);
};

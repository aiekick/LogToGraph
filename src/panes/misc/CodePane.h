#pragma once

#include <ezlibs/ezClass.hpp>
#include <ezlibs/ezSingleton.hpp>
#include <imguipack.h>
#include <frontend/Components/CodeEditor.h>
#include <apis/LtgPluginApi.h>  // also pulls IScriptDebugger.h transitively; brings ScriptingError + sc_PROJECT_SCRIPT_CHUNK
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

class ProjectFile;
class CodePane : public AbstractPane {
    DISABLE_CONSTRUCTORS(CodePane)
    DISABLE_DESTRUCTORS(CodePane)
    IMPLEMENT_SHARED_SINGLETON(CodePane)

private:
    struct CodeSheet {
        CodeEditor codeEditor;
        std::string filepathName;
        std::string title;
        bool wasModified = false;
        bool opened = false;
    };
    std::vector<CodeSheet> m_CodeSheets;
    // breakpoint render cache — refreshed only when ScriptDebugger::getBreakpointsRevision() advances
    int64_t m_BreakpointsRevisionSeen = -1;
    std::string m_DebugScriptFileCache;
    std::unordered_set<int32_t> m_Breakpoints0BasedCache;
    // paused state cache — refreshed only when ScriptDebugger::getStateRevision() advances (i.e. on a new pause)
    int64_t m_LastStateRevision = -1;
    Ltg::DebugState m_StateCache;
    // last-run scripting errors — refreshed when ScriptingEngine::GetErrorsRevision() advances
    int64_t m_LastErrorsRevisionSeen = -1;
    std::vector<Ltg::ScriptingError> m_ErrorsCache;
    // hover-eval state — set per-frame by the editor's hover callback, consumed after the tab bar
    // to draw a tooltip with the value evaluated in the paused frame. eval IDs start far above
    // WatcherPane's range so they never collide.
    std::string m_HoveredToken;
    std::string m_LastHoverEvalToken;
    int32_t m_HoverEvalId = (1 << 30);
    double m_MouseStillSince = 0.0;  // ImGui::GetTime() when the mouse last stopped moving — gates the hover tooltip

public:
    bool init() final;
    void unit() final;
    bool drawPanes(bool* apOpened, LayoutPaneUserDatas apUserDatas) final;

    void Clear();  // called by ProjectFile::ClearDatas on New/Open/Close

    void OpenFile(const std::string& vFilePathName, size_t vErrorLine = 0, std::string vErrorMsg = {});

    // the single in-app project script (stored in the .ltg db, not an external file)
    static constexpr const char* sc_PROJECT_SCRIPT_ID = "<project script>";
    void OpenScript(const std::string& aCode);
    std::string GetScriptCode();
    void MarkScriptSaved();  // editor undo baseline = current (called after a project save)

private:
    void m_DrawDebugToolbar();
    void m_StartAnalyse();
};

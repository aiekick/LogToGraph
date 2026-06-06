#pragma once

#include <ezlibs/ezClass.hpp>
#include <ezlibs/ezSingleton.hpp>
#include <imguipack.h>
#include <frontend/Components/CodeEditor.h>
#include <cstdint>
#include <memory>
#include <string>
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

public:
    bool init() final;
    void unit() final;
    bool drawPanes(bool* apOpened, LayoutPaneUserDatas apUserDatas) final;

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

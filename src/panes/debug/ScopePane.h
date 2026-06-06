#pragma once

#include <ezlibs/ezClass.hpp>
#include <ezlibs/ezSingleton.hpp>
#include <imguipack.h>
#include <apis/IScriptDebugger.h>

#include <cstdint>
#include <memory>
#include <string>

// Debug category pane: the variables in the CURRENT scope, i.e. the locals and
// upvalues that are live at the paused line (the innermost frame). Globals are
// out of scope here on purpose; the Stack Tree pane shows the full picture.
class ScopePane : public AbstractPane {
    DISABLE_CONSTRUCTORS(ScopePane)
    DISABLE_DESTRUCTORS(ScopePane)
    IMPLEMENT_SHARED_SINGLETON(ScopePane)

private:
    // paused state cache — refreshed only when ScriptDebugger::getStateRevision() advances
    int64_t m_LastStateRevision = -1;
    Ltg::DebugState m_StateCache;

public:
    bool init() override;
    void unit() override;
    bool drawPanes(bool* apOpened, LayoutPaneUserDatas apUserDatas) override;
    void Clear();  // called by ProjectFile::ClearDatas
};

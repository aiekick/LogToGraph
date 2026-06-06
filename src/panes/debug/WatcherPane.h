#pragma once

#include <ezlibs/ezClass.hpp>
#include <ezlibs/ezSingleton.hpp>
#include <imguipack.h>
#include <apis/IScriptDebugger.h>

#include <cstdint>
#include <string>
#include <vector>

// Debug category pane: a list of watch expressions evaluated in the paused frame's scope.
// Entries are added by right-clicking a token in the Code pane, or typed in manually at the top
// input row. Each entry carries a unique evalId that the host re-uses to dedupe requests; results
// are invalidated automatically on every new pause (see ScriptDebugger::onPause).
class WatcherPane : public AbstractPane {
    DISABLE_CONSTRUCTORS(WatcherPane)
    DISABLE_DESTRUCTORS(WatcherPane)
    IMPLEMENT_SHARED_SINGLETON(WatcherPane)

private:
    struct WatchEntry {
        std::string expression;
        int32_t evalId = -1;
        Ltg::EvalResult lastResult;
        bool hasResult = false;
    };
    std::vector<WatchEntry> m_Entries;
    int32_t m_NextEvalId = 0;            // monotonic; never re-used so a stale result never aliases
    char m_NewExpressionBuf[256] = "";   // manual-add input row at the top of the pane

public:
    bool init() override;
    void unit() override;
    bool drawPanes(bool* apOpened, LayoutPaneUserDatas apUserDatas) override;

    void Clear();  // called by ProjectFile::ClearDatas

    // adds a new watch expression (no dedup — the user may want the same expression twice).
    // safe to call from anywhere on the UI thread (right-click on a token routes here).
    void AddExpression(const std::string& aExpression);
};

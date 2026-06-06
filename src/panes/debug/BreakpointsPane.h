#pragma once

#include <ezlibs/ezClass.hpp>
#include <ezlibs/ezSingleton.hpp>
#include <imguipack.h>

#include <cstdint>
#include <memory>
#include <string>

// Debug category pane: lists the breakpoints held by the shared debugger.
class BreakpointsPane : public AbstractPane {
    DISABLE_CONSTRUCTORS(BreakpointsPane)
    DISABLE_DESTRUCTORS(BreakpointsPane)
    IMPLEMENT_SHARED_SINGLETON(BreakpointsPane)

public:
    bool init() override;
    void unit() override;
    bool drawPanes(bool* apOpened, LayoutPaneUserDatas apUserDatas) override;
};

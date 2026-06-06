#pragma once

#include <ezlibs/ezClass.hpp>
#include <ezlibs/ezSingleton.hpp>
#include <imguipack.h>

#include <cstdint>
#include <memory>
#include <string>

// Debug category pane: shows the call stack of the paused frame.
class CalltracePane : public AbstractPane {
    DISABLE_CONSTRUCTORS(CalltracePane)
    DISABLE_DESTRUCTORS(CalltracePane)
    IMPLEMENT_SHARED_SINGLETON(CalltracePane)

public:
    bool init() override;
    void unit() override;
    bool drawPanes(bool* apOpened, LayoutPaneUserDatas apUserDatas) override;
};

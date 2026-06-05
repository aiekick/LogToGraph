#pragma once

#include <ezlibs/ezClass.hpp>
#include <ezlibs/ezSingleton.hpp>
#include <imguipack.h>

#include <cstdint>
#include <memory>
#include <string>

// Debug category pane: shows the locals and upvalues of the paused frame.
class WatcherPane : public AbstractPane {
    DISABLE_CONSTRUCTORS(WatcherPane)
    DISABLE_DESTRUCTORS(WatcherPane)
    IMPLEMENT_SHARED_SINGLETON(WatcherPane)

public:
    bool init() override;
    void unit() override;
    bool drawPanes(bool* apOpened, LayoutPaneUserDatas apUserDatas) override;
};

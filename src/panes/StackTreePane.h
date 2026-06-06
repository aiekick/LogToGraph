#pragma once

#include <ezlibs/ezClass.hpp>
#include <ezlibs/ezSingleton.hpp>
#include <imguipack.h>

#include <cstdint>
#include <memory>
#include <string>

// Debug category pane: the whole Lua call stack as an explorable tree
// (Stack -> [i] frame -> Locals/Upvalues -> vars) plus the Globals.
class StackTreePane : public AbstractPane {
    DISABLE_CONSTRUCTORS(StackTreePane)
    DISABLE_DESTRUCTORS(StackTreePane)
    IMPLEMENT_SHARED_SINGLETON(StackTreePane)

public:
    bool init() override;
    void unit() override;
    bool drawPanes(bool* apOpened, LayoutPaneUserDatas apUserDatas) override;
};

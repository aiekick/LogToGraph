#pragma once

#include <ezlibs/ezClass.hpp>
#include <ezlibs/ezSingleton.hpp>
#include <imguipack.h>

#include <cstdint>
#include <memory>
#include <string>

class ProjectFile;
class ConsolePane : public AbstractPane {
    DISABLE_CONSTRUCTORS(ConsolePane)
    DISABLE_DESTRUCTORS(ConsolePane)
    IMPLEMENT_SHARED_SINGLETON(ConsolePane)

public:
    bool init() override;
    void unit() override;
    bool drawPanes(bool* apOpened, LayoutPaneUserDatas apUserDatas) override;
};

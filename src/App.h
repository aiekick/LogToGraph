#pragma once

#include <ezlibs/ezApp.hpp>
#include <ezlibs/ezClass.hpp>

class App : ez::App {
    DISABLE_CONSTRUCTORS(App)
    DISABLE_DESTRUCTORS(App)

public:
    App(int aArgc, char** apArgv);
    int run();

private:
    void m_InitMessaging();
    void m_InitSingletons();
    void m_UnitSingletons();
};

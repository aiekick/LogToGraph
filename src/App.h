#pragma once

#include <ezlibs/ezApp.hpp>

class App : ez::App {
public:
    App(int vArgc, char** vArgv);
    int run();

public:
    App() = default;           // Prevent construction
    virtual ~App() = default;  // Prevent unwanted destruction

private:
    void m_InitMessaging();

protected:
    App(const App&) = default;  // Prevent construction by copying
    App& operator=(const App&) { return *this; };  // Prevent assignment
};

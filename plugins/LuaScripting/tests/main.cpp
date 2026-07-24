// Custom gtest main: the plugin code logs through ez::Log, whose NEW_SINGLETON flavor
// must be installed explicitly (in production the host passes its own instance through
// PluginInterface::init(ez::Log*) — see LuaScripting.cpp). Without this, the first
// LogVar* call dereferences a null singleton and the error-path tests explode.

#include <ezlibs/ezLog.hpp>

#include <gtest/gtest.h>

int main(int argc, char** argv) {
    ez::Log::initSingleton(nullptr);  // own a fresh instance for the whole test run
    ::testing::InitGoogleTest(&argc, argv);
    const int result = RUN_ALL_TESTS();
    ez::Log::unitSingleton();
    return result;
}

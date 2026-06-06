// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include <headers/LuaScriptingBuild.h>
#include "LuaScripting.h"

#include <ezlibs/ezLog.hpp>

// needed for plugin creating / destroying
extern "C"  // needed for avoid renaming of funcs by the compiler
{
#ifdef WIN32
#define PLUGIN_PREFIX __declspec(dllexport)
#else
#define PLUGIN_PREFIX
#endif

PLUGIN_PREFIX LuaScripting* allocator() {
    return new LuaScripting();
}

PLUGIN_PREFIX void deleter(LuaScripting* ptr) {
    delete ptr;
}
}

#include <modules/Module.h>

LuaScripting::LuaScripting() = default;

bool LuaScripting::init(ez::Log* vLoggerInstancePtr) {
    // borrow the host's ez::Log so every LogVar* call from this DLL routes through
    // the host's standardLogFunctor (which pushes into the Messaging pane)
    ez::Log::initSingleton(vLoggerInstancePtr);
    return true;
}

void LuaScripting::unit() {
    ez::Log::unitSingleton();  // only releases the borrow — does NOT delete the host instance
}

uint32_t LuaScripting::getMinimalAppVersionSupported() const {
    return 0U;
}

uint32_t LuaScripting::getVersionMajor() const {
    return LuaScripting_MinorNumber;
}

uint32_t LuaScripting::getVersionMinor() const {
    return LuaScripting_MajorNumber;
}

uint32_t LuaScripting::getVersionBuild() const {
    return LuaScripting_BuildNumber;
}

std::string LuaScripting::getName() const {
    return "LuaScripting";
}

std::string LuaScripting::getAuthor() const {
    return "Stephane Cuillerdier";
}

std::string LuaScripting::getVersion() const {
    return LuaScripting_BuildId;
}

std::string LuaScripting::getContact() const {
    return "aiekick@funparadigm.com";
}

std::string LuaScripting::getDescription() const {
    return "Lua Scripting plugin for LogToGraph";
}

std::vector<Ltg::PluginModuleInfos> LuaScripting::getModulesInfos() const {
    std::vector<Ltg::PluginModuleInfos> res;
    res.push_back(Ltg::PluginModuleInfos("", "Lua", Ltg::PluginModuleType::SCRIPTING));
    return res;
}

Ltg::PluginModulePtr LuaScripting::createModule(const std::string& vPluginModuleName, Ltg::PluginBridge* vBridgePtr) {
    if (vPluginModuleName == "Lua") {
        return Module::create();
    }
    return nullptr;
}

// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include <Headers/LuaScriptingBuild.h>
#include "LuaScripting.h"

#define EZ_LOG_IMPLEMENTATION
#include <EzLibs/EzLog.hpp>

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

#include <Modules/Module.h>

LuaScripting::LuaScripting() {
#ifdef _MSC_VER
    // active memory leak detector
    //_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
}

bool LuaScripting::Init() {
    m_SettingsPtr = std::make_shared<Settings>();
    return true;
}

void LuaScripting::Unit() {
    m_SettingsPtr.reset();
}

uint32_t LuaScripting::GetMinimalStrockerVersionSupported() const {
    return 0U;
}

uint32_t LuaScripting::GetVersionMajor() const {
    return LuaScripting_MinorNumber;
}

uint32_t LuaScripting::GetVersionMinor() const {
    return LuaScripting_MajorNumber;
}

uint32_t LuaScripting::GetVersionBuild() const {
    return LuaScripting_BuildNumber;
}

std::string LuaScripting::GetName() const {
    return "LuaScripting";
}

std::string LuaScripting::GetAuthor() const {
    return "Stephane Cuillerdier";
}

std::string LuaScripting::GetVersion() const {
    return LuaScripting_BuildId;
}

std::string LuaScripting::GetContact() const {
    return "strocker@funparadigm.com";
}

std::string LuaScripting::GetDescription() const {
    return "Yahoo data broker";
}

std::vector<Ltg::PluginModuleInfos> LuaScripting::GetModulesInfos() const {
    std::vector<Ltg::PluginModuleInfos> res;
    res.push_back(Ltg::PluginModuleInfos("", "Lua", Ltg::PluginModuleType::SCRIPTING));
    return res;
}

Ltg::PluginModulePtr LuaScripting::CreateModule(const std::string& vPluginModuleName, Ltg::PluginBridge* vBridgePtr) {
    if (vPluginModuleName == "Lua") {
        return Module::create(m_SettingsPtr);
    }
    return nullptr;
}

std::vector<Ltg::PluginPaneConfig> LuaScripting::GetPanes() const {
    std::vector<Ltg::PluginPaneConfig> res;
    return res;
}

std::vector<Ltg::PluginSettingsConfig> LuaScripting::GetSettings() const {
    std::vector<Ltg::PluginSettingsConfig> res;
    res.push_back(Ltg::PluginSettingsConfig(m_SettingsPtr));
    return res;
}

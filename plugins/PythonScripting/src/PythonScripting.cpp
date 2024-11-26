// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include <Headers/PythonScriptingBuild.h>
#include "PythonScripting.h"

#define EZ_LOG_IMPLEMENTATION
#include <ezlibs/ezLog.hpp>

// needed for plugin creating / destroying
extern "C"  // needed for avoid renaming of funcs by the compiler
{
#ifdef WIN32
#define PLUGIN_PREFIX __declspec(dllexport)
#else
#define PLUGIN_PREFIX
#endif

PLUGIN_PREFIX PythonScripting* allocator() {
    return new PythonScripting();
}

PLUGIN_PREFIX void deleter(PythonScripting* ptr) {
    delete ptr;
}
}

#include <Modules/Module.h>

PythonScripting::PythonScripting() = default;

bool PythonScripting::init() {
    m_SettingsPtr = std::make_shared<Settings>();
    return true;
}

void PythonScripting::unit() {
    m_SettingsPtr.reset();
}

uint32_t PythonScripting::getMinimalAppVersionSupported() const {
    return 0U;
}

uint32_t PythonScripting::getVersionMajor() const {
    return PythonScripting_MinorNumber;
}

uint32_t PythonScripting::getVersionMinor() const {
    return PythonScripting_MajorNumber;
}

uint32_t PythonScripting::getVersionBuild() const {
    return PythonScripting_BuildNumber;
}

std::string PythonScripting::getName() const {
    return "PythonScripting";
}

std::string PythonScripting::getAuthor() const {
    return "Stephane Cuillerdier";
}

std::string PythonScripting::getVersion() const {
    return PythonScripting_BuildId;
}

std::string PythonScripting::getContact() const {
    return "aiekick@funparadigm.com";
}

std::string PythonScripting::getDescription() const {
    return "Python Scripting plugin for LogToGraph";
}

std::vector<Ltg::PluginModuleInfos> PythonScripting::getModulesInfos() const {
    std::vector<Ltg::PluginModuleInfos> res;
    res.push_back(Ltg::PluginModuleInfos("", "Python", Ltg::PluginModuleType::SCRIPTING));
    return res;
}

Ltg::PluginModulePtr PythonScripting::createModule(const std::string& vPluginModuleName, Ltg::PluginBridge* vBridgePtr) {
    if (vPluginModuleName == "Python") {
        return Module::create(m_SettingsPtr);
    }
    return nullptr;
}

std::vector<Ltg::PluginPaneConfig> PythonScripting::getPanes() const {
    std::vector<Ltg::PluginPaneConfig> res;
    return res;
}

std::vector<Ltg::PluginSettingsConfig> PythonScripting::getSettings() const {
    std::vector<Ltg::PluginSettingsConfig> res;
    res.push_back(Ltg::PluginSettingsConfig(m_SettingsPtr));
    return res;
}

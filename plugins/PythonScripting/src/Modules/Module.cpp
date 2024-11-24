#include "Module.h"
#include <ImGuiPack.h>
#include <EzLibs/EzFile.hpp>
#include <EzLibs/EzTime.hpp>

#include <chrono>
#include <ctime>

///////////////////////////////////////////////////
/// CUSTOM PYTHON FUNCTIONS ///////////////////////
///////////////////////////////////////////////////

///////////////////////////////////////////////////
///////////////////////////////////////////////////
///////////////////////////////////////////////////

Ltg::ScriptingModulePtr Module::create(const SettingsWeak& vSettings) {
    assert(!vSettings.expired());
    auto res = std::make_shared<Module>();
    res->m_Settings = vSettings;
    if (!res->init()) {
        res.reset();
    }
    return res;
}

bool Module::init(Ltg::PluginBridge* vBridgePtr) {
    return true;
}

void Module::unit() {
}

bool Module::load() {
    return true;
}

void Module::unload() {
}

bool Module::compileScript(const Ltg::ScriptFilePathName& vFilePathName, Ltg::ErrorContainer& vOutErrors) {
    return true;
}
bool Module::callScriptInit(Ltg::ErrorContainer& vOutErrors) {
    return true;
}
bool Module::callScriptStart(Ltg::ErrorContainer& vOutErrors) {
    return true;
}
bool Module::callScriptExec(const Ltg::ScriptingDatas& vOutDatas, Ltg::ErrorContainer& vErrors) {
    return true;
}
bool Module::callScriptEnd(Ltg::ErrorContainer& vOutErrors) {
    return true;
}

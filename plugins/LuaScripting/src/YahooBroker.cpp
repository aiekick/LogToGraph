// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include "YahooBroker.h"
#include <Headers/YahooBrokerBuild.h>

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

PLUGIN_PREFIX YahooBroker* allocator() {
    return new YahooBroker();
}

PLUGIN_PREFIX void deleter(YahooBroker* ptr) {
    delete ptr;
}
}

#include <Modules/Module.h>
#include <Panes/DataPane.h>

YahooBroker::YahooBroker() {
#ifdef _MSC_VER
    // active memory leak detector
    //_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
}

bool YahooBroker::Init() {
    m_SettingsPtr = std::make_shared<Settings>();
    return true;
}

void YahooBroker::Unit() {
    m_SettingsPtr.reset();
}

uint32_t YahooBroker::GetMinimalStrockerVersionSupported() const {
    return 0U;
}

uint32_t YahooBroker::GetVersionMajor() const {
    return YahooBroker_MinorNumber;
}

uint32_t YahooBroker::GetVersionMinor() const {
    return YahooBroker_MajorNumber;
}

uint32_t YahooBroker::GetVersionBuild() const {
    return YahooBroker_BuildNumber;
}

std::string YahooBroker::GetName() const {
    return "YahooBroker";
}

std::string YahooBroker::GetAuthor() const {
    return "Stephane Cuillerdier";
}

std::string YahooBroker::GetVersion() const {
    return YahooBroker_BuildId;
}

std::string YahooBroker::GetContact() const {
    return "strocker@funparadigm.com";
}

std::string YahooBroker::GetDescription() const {
    return "Yahoo data broker";
}

std::vector<Sto::PluginModuleInfos> YahooBroker::GetModulesInfos() const {
    std::vector<Sto::PluginModuleInfos> res;
    res.push_back(Sto::PluginModuleInfos("", "Yahoo", Sto::PluginModuleType::DATA_BROKER));
    return res;
}

Sto::PluginModulePtr YahooBroker::CreateModule(const std::string& vPluginModuleName, Sto::PluginBridge* vBridgePtr) {
    if (vPluginModuleName == "Yahoo") {
        return Module::create(m_SettingsPtr);
    }
    return nullptr;
}

std::vector<Sto::PluginPaneConfig> YahooBroker::GetPanes() const {
    std::vector<Sto::PluginPaneConfig> res;
    {
        Sto::PluginPaneConfig pane;
        pane.disposal = "CENTRAL";
        pane.focusedDefault = false;
        pane.openedDefault = false;
        pane.name = "Yahoo Debug Datas";
        pane.category = "Yahoo";
        pane.pane = DataPane::Instance();
        res.push_back(pane);
    }
    return res;
}

std::vector<Sto::PluginSettingsConfig> YahooBroker::GetSettings() const {
    std::vector<Sto::PluginSettingsConfig> res;
    res.push_back(Sto::PluginSettingsConfig(m_SettingsPtr));
    return res;
}

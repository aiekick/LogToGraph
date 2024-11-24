#pragma once

#include <apis/LtgPluginApi.h>

#include <Settings/Settings.h>

#include <ImGuiPack.h>

#include <string>
#include <vector>

class Module : public Ltg::ScriptingModule {
public:
    static Ltg::ScriptingModulePtr create(const SettingsWeak& vSettings);

private:
    SettingsWeak m_Settings;

public:
    virtual ~Module() = default;
    bool init(Ltg::PluginBridge* vBridgePtr = nullptr) final;
    void unit() final;

    bool load() final;
    void unload() final;
    bool compileScript(const Ltg::ScriptFilePathName& vFilePathName, Ltg::ErrorContainer& vOutErrors) final;
    bool callScriptInit(Ltg::ErrorContainer& vOutErrors) final;
    bool callScriptStart(Ltg::ErrorContainer& vOutErrors) final;
    bool callScriptExec(const Ltg::ScriptingDatas& vOutDatas, Ltg::ErrorContainer& vErrors) final;
    bool callScriptEnd(Ltg::ErrorContainer& vOutErrors) final;

private:

};

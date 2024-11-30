#pragma once

#include <modules/LuaDatasModel.h>
#include <apis/LtgPluginApi.h>
#include <settings/Settings.h>
#include <string>
#include <vector>
#include <memory>

namespace sol {
class state;
}

//struct lua_State;
class Module : public Ltg::ScriptingModule {
public:
    static Ltg::ScriptingModulePtr create(const SettingsWeak& vSettings);

private:
    std::unique_ptr<sol::state> m_luaPtr = nullptr;
    SettingsWeak m_settings;
    Ltg::IDatasModelWeak m_datasModel;
    LuaDatasModelPtr m_luaDatasModelPtr = nullptr;

public:
    virtual ~Module() = default;
    bool init(Ltg::PluginBridge* vBridgePtr = nullptr) final;
    void unit() final;

    bool load(Ltg::IDatasModelWeak vDatasModel) final;
    void unload() final;
    bool compileScript(const Ltg::ScriptFilePathName& vFilePathName, Ltg::ErrorContainer& vOutErrors) final;
    bool callScriptStart(Ltg::ErrorContainer& vOutErrors) final;
    bool callScriptExec(const Ltg::ScriptingDatas& vOutDatas, Ltg::ErrorContainer& vErrors) final;
    bool callScriptEnd(Ltg::ErrorContainer& vOutErrors) final;

    void setRowIndex(int32_t vRowIndex) final;
    void setRowCount(int32_t vRowCount) final;
};

#pragma once

#include <apis/LtgPluginApi.h>
#include <Settings/Settings.h>

class LuaScripting : public Ltg::PluginInterface {
private:
    SettingsPtr m_SettingsPtr = nullptr;  // common eettings for whole module

public:
    LuaScripting();
    virtual ~LuaScripting() = default;
    bool Init() override;
    void Unit() override;
    uint32_t GetMinimalStrockerVersionSupported() const override;
    uint32_t GetVersionMajor() const override;
    uint32_t GetVersionMinor() const override;
    uint32_t GetVersionBuild() const override;
    std::string GetName() const override;
    std::string GetAuthor() const override;
    std::string GetVersion() const override;
    std::string GetContact() const override;
    std::string GetDescription() const override;
    std::vector<Ltg::PluginModuleInfos> GetModulesInfos() const override;
    Ltg::PluginModulePtr CreateModule(const std::string& vPluginModuleName, Ltg::PluginBridge* vBridgePtr) override;
    std::vector<Ltg::PluginPaneConfig> GetPanes() const override;
    std::vector<Ltg::PluginSettingsConfig> GetSettings() const override;
};
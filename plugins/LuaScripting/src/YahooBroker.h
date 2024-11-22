#pragma once

#include <apis/StrockerPluginApi.h>
#include <Settings/Settings.h>

class YahooBroker : public Sto::PluginInterface {
private:
    SettingsPtr m_SettingsPtr = nullptr;  // common eettings for whole module

public:
    YahooBroker();
    virtual ~YahooBroker() = default;
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
    std::vector<Sto::PluginModuleInfos> GetModulesInfos() const override;
    Sto::PluginModulePtr CreateModule(const std::string& vPluginModuleName, Sto::PluginBridge* vBridgePtr) override;
    std::vector<Sto::PluginPaneConfig> GetPanes() const override;
    std::vector<Sto::PluginSettingsConfig> GetSettings() const override;
};
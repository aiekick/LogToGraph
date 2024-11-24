#pragma once

#include <apis/LtgPluginApi.h>
#include <Settings/Settings.h>

class PythonScripting : public Ltg::PluginInterface {
private:
    SettingsPtr m_SettingsPtr = nullptr;  // common eettings for whole module

public:
    PythonScripting();
    virtual ~PythonScripting() = default;
    bool init() override;
    void unit() override;
    uint32_t getMinimalAppVersionSupported() const override;
    uint32_t getVersionMajor() const override;
    uint32_t getVersionMinor() const override;
    uint32_t getVersionBuild() const override;
    std::string getName() const override;
    std::string getAuthor() const override;
    std::string getVersion() const override;
    std::string getContact() const override;
    std::string getDescription() const override;
    std::vector<Ltg::PluginModuleInfos> getModulesInfos() const override;
    Ltg::PluginModulePtr createModule(const std::string& vPluginModuleName, Ltg::PluginBridge* vBridgePtr) override;
    std::vector<Ltg::PluginPaneConfig> getPanes() const override;
    std::vector<Ltg::PluginSettingsConfig> getSettings() const override;
};
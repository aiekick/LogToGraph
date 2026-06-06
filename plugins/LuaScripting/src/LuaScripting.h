#pragma once

#include <apis/LtgPluginApi.h>

class LuaScripting : public Ltg::PluginInterface {
public:
    LuaScripting();
    virtual ~LuaScripting() = default;
    bool init(ez::Log* vLoggerInstancePtr) override;
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
};

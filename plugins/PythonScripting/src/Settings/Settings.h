#pragma once

#include <apis/LtgPluginApi.h>

class Settings : public Ltg::ISettings {
private:  // load / save
    std::string m_ApiKey;

private: // for imgui
    std::array<char, 2048 + 1> m_TmpBuffer = {};

public: // for Yahoo
    std::string getApiKey() const;

public: // for Strocker
    Ltg::SettingsCategoryPath getCategory() const override;
    bool loadSettings() override;
    bool saveSettings() override; 
    bool drawSettings() override;
    ez::xml::Nodes getXmlSettings(const Ltg::ISettingsType& vType) const final;
    void setXmlSettings(const ez::xml::Node& vName, const ez::xml::Node& vParent, const std::string& vValue, const Ltg::ISettingsType& vType) final;
};

typedef std::shared_ptr<Settings> SettingsPtr;
typedef std::weak_ptr<Settings> SettingsWeak;
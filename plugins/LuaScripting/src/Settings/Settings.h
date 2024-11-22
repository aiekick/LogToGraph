#pragma once

#include <apis/StrockerPluginApi.h>

class Settings : public Sto::ISettings {
private:  // load / save
    std::string m_ApiKey;

private: // for imgui
    std::array<char, 2048 + 1> m_TmpBuffer = {};

public: // for Yahoo
    std::string getApiKey() const;

public: // for Strocker
    Sto::SettingsCategoryPath GetCategory() const override;
    bool LoadSettings() override;
    bool SaveSettings() override; 
    bool DrawSettings() override;                    
    std::string GetXmlSettings(const std::string& vOffset, const Sto::ISettingsType& vType) const override;
    void SetXmlSettings(const std::string& vName, const std::string& vParentName, const std::string& vValue, const Sto::ISettingsType& vType) override;
};

typedef std::shared_ptr<Settings> SettingsPtr;
typedef std::weak_ptr<Settings> SettingsWeak;
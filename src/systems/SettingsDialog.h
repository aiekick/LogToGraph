#pragma once

#include <ezlibs/ezXmlConfig.hpp>
#include <apis/StrockerPluginApi.h>

class SettingsDialog : public ez::xml::Config {
public:
    std::map<Sto::SettingsCategoryPath, Sto::ISettingsWeak> m_SettingsPerCategoryPath;
    bool m_ShowDialog = false;
    Sto::SettingsCategoryPath m_SelectedCategoryPath;

public:
    bool init();
    void unit();

    void OpenDialog();
    void CloseDialog();

    bool Draw();

    std::string getXml(const std::string& vOffset, const std::string& vUserDatas = "") override;
    bool setFromXml(const ez::xml::Node& vNode, const ez::xml::Node& vParent, const std::string& vUserDatas) override;

private:
    void m_DrawCategoryPanes();
    void m_DrawContentPane();
    void m_DrawButtonsPane();
    bool m_Load();
    bool m_Save();

public:  // singleton
    static SettingsDialog* Instance() {
        static SettingsDialog _instance;
        return &_instance;
    }
};

#pragma once

#include <ezlibs/ezXmlConfig.hpp>
#include <apis/LtgPluginApi.h>

class SettingsDialog : public ez::xml::Config {
public:
    std::map<Ltg::SettingsCategoryPath, Ltg::ISettingsWeak> m_SettingsPerCategoryPath;
    bool m_ShowDialog = false;
    Ltg::SettingsCategoryPath m_SelectedCategoryPath;

public:
    bool init();
    void unit();

    void OpenDialog();
    void CloseDialog();

    bool Draw();

    ez::xml::Nodes getXmlNodes(const std::string& vUserDatas = "") final;
    bool setFromXmlNodes(const ez::xml::Node& vNode, const ez::xml::Node& vParent, const std::string& vUserDatas) final;

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

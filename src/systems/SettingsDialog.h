#pragma once

#include <ezlibs/ezClass.hpp>
#include <ezlibs/ezSingleton.hpp>
#include <ezlibs/ezXmlConfig.hpp>
#include <systems/ISettings.h>

class SettingsDialog : public ez::xml::Config {
    DISABLE_CONSTRUCTORS(SettingsDialog)
    DISABLE_DESTRUCTORS(SettingsDialog)
    IMPLEMENT_SINGLETON(SettingsDialog)

public:
    std::map<Ltg::SettingsCategoryPath, Ltg::ISettingsWeak> m_SettingsPerCategoryPath;
    bool m_ShowDialog = false;
    Ltg::ISettingsWeak m_SelectedSettings;  // cached on left-pane click; content pane just locks + draws

public:
    bool init();
    void unit();

    void OpenDialog();
    void CloseDialog();

    bool Draw();

    // forwards to every registered ISettings — called by ProjectFile::ClearDatas on New/Open/Close
    void clearProjectSettings();

    ez::xml::Nodes getXmlNodes(const std::string& vUserDatas = "") final;
    bool setFromXmlNodes(const ez::xml::Node& vNode, const ez::xml::Node& vParent, const std::string& vUserDatas) final;

private:
    void m_DrawCategoryPanes();
    void m_DrawContentPane();
    void m_DrawButtonsPane();
    bool m_Load();
    bool m_Save();
};

#include <Systems/SettingsDialog.h>

#include <systems/PluginManager.h>

#include <ImGuiPack/ImGuiPack.h>

#include <Project/ProjectFile.h>

bool SettingsDialog::init() {
    const auto& pluginSettings = PluginManager::Instance()->getPluginSettings();
    for (const auto& s : pluginSettings) {
        auto ptr = s.settings.lock();
        if (ptr != nullptr) {
            m_SettingsPerCategoryPath[ptr->GetCategory()] = s.settings;
        }
    }
    return true;
}

void SettingsDialog::unit() {
    m_SettingsPerCategoryPath.clear();
}

void SettingsDialog::OpenDialog() {
    if (m_ShowDialog) {
        return;
    }
    m_Load();
    m_ShowDialog = true;
}

void SettingsDialog::CloseDialog() {
    m_ShowDialog = false;
}

bool SettingsDialog::Draw() {
    if (m_ShowDialog) {
        ImGui::Begin("Settings");
        {
            ImGui::Separator();
            m_DrawCategoryPanes();
            ImGui::SameLine();
            ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
            ImGui::SameLine();
            m_DrawContentPane();
            ImGui::Separator();
            m_DrawButtonsPane();
        }
        ImGui::End();
        return true;
    }

    return false;
}

void SettingsDialog::m_DrawCategoryPanes() {
    const auto size = ImGui::GetContentRegionMax() - ImVec2(100, 68);

    ImGui::BeginChild("Categories", ImVec2(100, size.y));

    ImGui::EndChild();
}

void SettingsDialog::m_DrawContentPane() {
    auto size = ImGui::GetContentRegionMax() - ImVec2(100, 68);

    if (!ImGui::GetCurrentWindow()->ScrollbarY) {
        size.x -= ImGui::GetStyle().ScrollbarSize;
    }

    ImGui::BeginChild("##Content", size);

    for (const auto& cat : m_SettingsPerCategoryPath) {
        auto ptr = cat.second.lock();
        if (ptr != nullptr) {
            ptr->DrawSettings();
        }
    }

    ImGui::EndChild();
}

void SettingsDialog::m_DrawButtonsPane() {
    if (ImGui::ContrastedButton("Ok")) {
        m_Save();
        CloseDialog();
    }
    ImGui::SameLine();
    if (ImGui::ContrastedButton("Cancel")) {
        CloseDialog();
    }
}

bool SettingsDialog::m_Load() {
    for (const auto& cat : m_SettingsPerCategoryPath) {
        auto ptr = cat.second.lock();
        if (ptr != nullptr) {
            ptr->LoadSettings();
        }
    }
    return false;
}

bool SettingsDialog::m_Save() {
    for (const auto& cat : m_SettingsPerCategoryPath) {
        auto ptr = cat.second.lock();
        if (ptr != nullptr) {
            ptr->SaveSettings();
        }
    }
    ProjectFile::Instance()->SetProjectChange();
    return false;
}

ez::xml::Nodes SettingsDialog::getXmlNodes(const std::string& vUserDatas) {
    ez::xml::Node node("plugins");
    for (const auto& cat : m_SettingsPerCategoryPath) {
        auto ptr = cat.second.lock();
        if (ptr != nullptr) {
            if (vUserDatas == "app") {
                node.addChilds(ptr->GetXmlSettings(Ltg::ISettingsType::APP));
            } else if (vUserDatas == "project") {
                node.addChilds(ptr->GetXmlSettings(Ltg::ISettingsType::PROJECT));
            } else {
                EZ_TOOLS_DEBUG_BREAK;  // ERROR
            }
        }
    }
    return {node};
}

bool SettingsDialog::setFromXmlNodes(const ez::xml::Node& vNode, const ez::xml::Node& vParent, const std::string& vUserDatas) {
    const auto& strName = vNode.getName();
    const auto& strValue = vNode.getContent();
    const auto& strParentName = vParent.getName();
    for (const auto& cat : m_SettingsPerCategoryPath) {
        auto ptr = cat.second.lock();
        if (ptr != nullptr) {
            if (vUserDatas == "app") {
                ptr->SetXmlSettings(strName, strParentName, strValue, Ltg::ISettingsType::APP);
                RecursParsingConfigChilds(vNode, vUserDatas);
            } else if (vUserDatas == "project") {
                ptr->SetXmlSettings(strName, strParentName, strValue, Ltg::ISettingsType::PROJECT);
                RecursParsingConfigChilds(vNode, vUserDatas);
            } else {
                EZ_TOOLS_DEBUG_BREAK;  // ERROR
            }
        }
    }
    return false;
}
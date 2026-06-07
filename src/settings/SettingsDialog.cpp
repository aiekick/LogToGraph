#include <settings/SettingsDialog.h>

#include <imguipack.h>

#include <project/ProjectFile.h>
#include <settings/AppSettings.h>
#include <settings/DebugSettings.h>

#include <cctype>
#include <string>

namespace {
// "app/general" -> "General" — the last path segment with its first letter uppercased.
std::string makeSectionLabel(const Ltg::SettingsCategoryPath& aPath) {
    const auto slash = aPath.find_last_of('/');
    std::string segment = (slash == std::string::npos) ? aPath : aPath.substr(slash + 1);
    if (!segment.empty()) {
        segment[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(segment[0])));
    }
    return segment;
}
}  // namespace

bool SettingsDialog::init() {
    // app-level settings — first host-side ISettings implementer. More can be registered the same way.
    m_Settings.push_back(AppSettings::ref());
    // per-feature debug toggles — shown under "Debug" in the left panel, mirrored in the CodePane Debug submenu.
    m_Settings.push_back(DebugSettings::ref());
    // pre-select the first entry so the content pane is non-empty on first open
    if (!m_Settings.empty()) {
        m_SelectedSettings = m_Settings.front();
    }
    return true;
}

void SettingsDialog::unit() {
    m_Settings.clear();
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

void SettingsDialog::clearProjectSettings() {
    for (const auto& cat : m_Settings) {
        auto ptr = cat.lock();
        if (ptr != nullptr) {
            ptr->clearProjectSettings();
        }
    }
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

    const auto selectedPtr = m_SelectedSettings.lock();
    for (const auto& cat : m_Settings) {
        const auto ptr = cat.lock();
        if (ptr != nullptr) {
            const auto label = makeSectionLabel(ptr->getCategory());
            const bool selected = (ptr == selectedPtr);
            if (ImGui::Selectable(label.c_str(), selected)) {
                m_SelectedSettings = cat;
            }
        }
    }

    ImGui::EndChild();
}

void SettingsDialog::m_DrawContentPane() {
    auto size = ImGui::GetContentRegionMax() - ImVec2(100, 68);

    if (!ImGui::GetCurrentWindow()->ScrollbarY) {
        size.x -= ImGui::GetStyle().ScrollbarSize;
    }

    ImGui::BeginChild("##Content", size);

    auto ptr = m_SelectedSettings.lock();
    if (ptr != nullptr) {
        ptr->drawSettings();
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
    for (const auto& cat : m_Settings) {
        auto ptr = cat.lock();
        if (ptr != nullptr) {
            ptr->loadSettings();
        }
    }
    return false;
}

bool SettingsDialog::m_Save() {
    for (const auto& cat : m_Settings) {
        auto ptr = cat.lock();
        if (ptr != nullptr) {
            ptr->saveSettings();
        }
    }
    ProjectFile::ref()->SetProjectChange();
    return false;
}

ez::xml::Nodes SettingsDialog::getXmlNodes(const std::string& vUserDatas) {
    ez::xml::Node node("plugins");
    for (const auto& cat : m_Settings) {
        auto ptr = cat.lock();
        if (ptr != nullptr) {
            if (vUserDatas == "app") {
                node.addChilds(ptr->getXmlSettings(Ltg::ISettingsType::APP));
            } else if (vUserDatas == "project") {
                node.addChilds(ptr->getXmlSettings(Ltg::ISettingsType::PROJECT));
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
    for (const auto& cat : m_Settings) {
        auto ptr = cat.lock();
        if (ptr != nullptr) {
            if (vUserDatas == "app") {
                ptr->setXmlSettings(strName, strParentName, strValue, Ltg::ISettingsType::APP);
            } else if (vUserDatas == "project") {
                ptr->setXmlSettings(strName, strParentName, strValue, Ltg::ISettingsType::PROJECT);
            } else {
                EZ_TOOLS_DEBUG_BREAK;  // ERROR
            }
        }
    }
    return true;
}
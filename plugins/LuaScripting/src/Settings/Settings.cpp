#include <Settings/Settings.h>

#include <ImGuiPack.h>

std::string Settings::getApiKey() const {
    return m_ApiKey;
}

Ltg::SettingsCategoryPath Settings::GetCategory() const {
    return "Brockers/Yahoo";
}

bool Settings::LoadSettings() {
    if (!m_ApiKey.empty()) {
        size_t maxCountChars = m_TmpBuffer.size();
        if (m_ApiKey.size() < maxCountChars) {
            maxCountChars = m_ApiKey.size();
        }
#ifdef _MSC_VER
        strncpy_s(m_TmpBuffer.data(), m_TmpBuffer.size(), m_ApiKey.c_str(), maxCountChars);
#else
        strncpy(m_TmpBuffer.data(), m_ApiKey.c_str(), maxCountChars);
#endif
    } else {
        m_TmpBuffer = {};
    }
    return false;
}

bool Settings::SaveSettings() {
    m_ApiKey = m_TmpBuffer.data();
    m_TmpBuffer[0] = '\0';
    return true;
}

bool Settings::DrawSettings() {
    bool change = false;
    ImGui::Header("Yahoo");
    //ImGui::Text("Api Key :");
    //change |= ImGui::InputText("##ApiKey", m_TmpBuffer.data(), m_TmpBuffer.size());
    return change;
}

ez::xml::Nodes Settings::GetXmlSettings(const Ltg::ISettingsType& vType) const {
    ez::xml::Node node("LuaScripting");
    return {node};
}

void Settings::SetXmlSettings(const ez::xml::Node& vName, const ez::xml::Node& vParent, const std::string& vValue, const Ltg::ISettingsType& vType) {
    if (!vName.getName().empty() && !vValue.empty()) {
        if (vParent.getName() == "LuaScripting") {
        }
    }
}

#include <Settings/Settings.h>

#include <ImGuiPack.h>

std::string Settings::getApiKey() const {
    return m_ApiKey;
}

Sto::SettingsCategoryPath Settings::GetCategory() const {
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

std::string Settings::GetXmlSettings(const std::string& vOffset, const Sto::ISettingsType& vType) const {
    std::string str;
    str += vOffset + "<Yahoo>\n";
    /*if (vType == Sto::ISettingsType::PROJECT) {  // we are considering than the api key is per user
        str += vOffset + "\t<api_key>" + m_ApiKey + "</api_key>\n";
    }*/
    str += vOffset + "</Yahoo>\n";
    return str;
}

void Settings::SetXmlSettings(const std::string& vName, const std::string& vParentName, const std::string& vValue, const Sto::ISettingsType& vType) {
    if (!vName.empty() && !vValue.empty()) {
        if (vParentName == "Yahoo") {
            /*if (vType == Sto::ISettingsType::PROJECT) {  // we are considering than the api key is per user
                if (vName == "api_key") {
                    m_ApiKey = vValue;
                }
            }*/
        }
    }
}

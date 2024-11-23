#pragma once

#include <json.hpp>
#include <apis/StrockerPluginApi.h>

#include <ImGuiPack.h>

#include <cstdint>
#include <memory>
#include <string>

class ProjectFile;
class DataPane : public AbstractPane {
private: // datas
    std::string m_RequestRawDatas;
    nlohmann::json m_RequestJsonDatas = {};
    Sto::SymbolPrices m_RequestPricesDatas;
    bool m_NeedToShowPane = false;

private:  // diplay
    ImGuiListClipper m_DataListClipper;

public:
    bool Init() override;
    void Unit() override;
    bool DrawWidgets(const uint32_t& vCurrentFrame, ImGuiContext* vContextPtr = nullptr, void* vUserDatas = nullptr) override;
    bool DrawOverlays(
        const uint32_t& vCurrentFrame, const ImRect& vRect, ImGuiContext* vContextPtr = nullptr, void* vUserDatas = nullptr) override;
    bool DrawPanes(
        const uint32_t& vCurrentFrame, bool* vOpened = nullptr, ImGuiContext* vContextPtr = nullptr, void* vUserDatas = nullptr) override;
    bool DrawDialogsAndPopups(
        const uint32_t& vCurrentFrame, const ImRect& vMaxRect, ImGuiContext* vContextPtr = nullptr, void* vUserDatas = nullptr) override;

    void setRequestDatasRaw(const std::string& vDatas);
    void setRequestDatasJson(const nlohmann::json& vDatas);
    void setRequestDatasPrices(const Sto::SymbolPrices& vDatas);

private:
    void m_DrawRawDatas();
    void m_DrawJsonDatas();
    void m_DrawPricesTableDatas();

public:  // singleton
    static std::shared_ptr<DataPane> Instance() {
        static std::shared_ptr<DataPane> _instance = std::make_shared<DataPane>();
        return _instance;
    }

public:
    DataPane();                              // Prevent construction
    DataPane(const DataPane&) = default;  // Prevent construction by copying
    DataPane& operator=(const DataPane&) {
        return *this;
    };                       // Prevent assignment
    virtual ~DataPane();  // Prevent unwanted destruction};
};

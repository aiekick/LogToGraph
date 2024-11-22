// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include "DataPane.h"
#include <ImGuiPack.h>
#include <cinttypes>  // printf zu

#include <EzLibs/EzStr.hpp>

DataPane::DataPane() = default;
DataPane::~DataPane() {
    Unit();
}

bool DataPane::Init() {
    return true;
}

void DataPane::Unit() {
}

///////////////////////////////////////////////////////////////////////////////////
//// IMGUI PANE ///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////

bool DataPane::DrawPanes(const uint32_t& /*vCurrentFrame*/, bool* vOpened, ImGuiContext* vContextPtr, void* /*vUserDatas*/) {
    ImGui::SetCurrentContext(vContextPtr);
    bool change = false;
    if (vOpened != nullptr) {
        if (*vOpened) {
            static ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus /* | ImGuiWindowFlags_MenuBar*/;
            if (ImGui::Begin(GetName().c_str(), vOpened, flags)) {
#ifdef USE_DECORATIONS_FOR_RESIZE_CHILD_WINDOWS
                auto win = ImGui::GetCurrentWindowRead();
                if (win->Viewport->Idx != 0)
                    flags |= ImGuiWindowFlags_NoResize;  // | ImGuiWindowFlags_NoTitleBar;
                else
                    flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus /* | ImGuiWindowFlags_MenuBar*/;
#endif
                if (ImGui::BeginMenuBar()) {
                    ImGui::EndMenuBar();
                }

                if (!m_RequestPricesDatas.symbol.empty()) {
                    ImGui::Text("Symbol : %s::%s Count : %u",
                                m_RequestPricesDatas.market.c_str(),
                                m_RequestPricesDatas.symbol.c_str(),
                                (uint32_t)m_RequestPricesDatas.prices.times.size());
                    if (ImGui::BeginTabBar("Datas Response")) {
                        if (ImGui::BeginTabItem("Curl Raw")) {
                            m_DrawRawDatas();
                            ImGui::EndTabItem();
                        }
                        if (ImGui::BeginTabItem("Json")) {
                            m_DrawJsonDatas();
                            ImGui::EndTabItem();
                        }
                        if (ImGui::BeginTabItem("Prices Tables")) {
                            m_DrawPricesTableDatas();
                            ImGui::EndTabItem();
                        }
                        ImGui::EndTabBar();
                    }
                }
            }
            ImGui::End();
        }
        if (m_NeedToShowPane) {
            *vOpened = true;
            m_NeedToShowPane = false;
        }
    }
    return change;
}

bool DataPane::DrawOverlays(const uint32_t& /*vCurrentFrame*/, const ImRect& /*vRect*/, ImGuiContext* vContextPtr, void* /*vUserDatas*/) {
    ImGui::SetCurrentContext(vContextPtr);
    return false;
}

bool DataPane::DrawDialogsAndPopups(const uint32_t& /*vCurrentFrame*/, const ImRect& /*vMaxRect*/, ImGuiContext* vContextPtr, void* /*vUserDatas*/) {
    ImGui::SetCurrentContext(vContextPtr);
    return false;
}

bool DataPane::DrawWidgets(const uint32_t& /*vCurrentFrame*/, ImGuiContext* vContextPtr, void* /*vUserDatas*/) {
    ImGui::SetCurrentContext(vContextPtr);
    return false;
}

void DataPane::setRequestDatasRaw(const std::string& vDatas) {
    m_RequestRawDatas = vDatas;
    m_NeedToShowPane = true; // will start showing the pane
}

void DataPane::setRequestDatasJson(const nlohmann::json& vDatas) {
    m_RequestJsonDatas = vDatas;
    m_NeedToShowPane = true;  // will start showing the pane
}
void DataPane::setRequestDatasPrices(const Sto::SymbolPrices& vDatas) {
    m_RequestPricesDatas = vDatas;
    m_NeedToShowPane = true;  // will start showing the pane
}

/////////////////////////////////////////////////
///// PRIVATE ///////////////////////////////////
/////////////////////////////////////////////////

void DataPane::m_DrawRawDatas() {
    if (ImGui::BeginChild("##YahooRawDatas")) {
        if (!m_RequestRawDatas.empty()) {
            ImGui::TextWrapped("%s", m_RequestRawDatas.c_str());
        }
    }
    ImGui::EndChild();
}

void DataPane::m_DrawJsonDatas() {

}

void DataPane::m_DrawPricesTableDatas() {
    ImGui::PushID(this);
    static ImGuiTableFlags flags =        //
        ImGuiTableFlags_SizingFixedFit |  //
        ImGuiTableFlags_RowBg |           //
        ImGuiTableFlags_Hideable |        //
        ImGuiTableFlags_ScrollY |         //
        ImGuiTableFlags_NoHostExtendY;    //
    if (ImGui::BeginTable("##PricesTable", 7)) {
        ImGui::TableSetupScrollFreeze(0, 1);  // Make header always visible
        ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("Epoch", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("Open", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("Close", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("High", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("Low", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("Volume", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableHeadersRow();
        m_DataListClipper.Begin((int32_t)m_RequestPricesDatas.prices.times.size(), ImGui::GetTextLineHeightWithSpacing());
        while (m_DataListClipper.Step()) {
            for (int32_t i = m_DataListClipper.DisplayStart; i < m_DataListClipper.DisplayEnd; ++i) {
                if (i < 0) {
                    continue;
                }
                const auto& idx = (size_t)i;
                ImGui::TableNextRow();
                if (ImGui::TableNextColumn()) {  // row
                    ImGui::Selectable(ez::str::toStr("%i", i).c_str());
                }
                if (ImGui::TableNextColumn()) {  // epoch
                    ImGui::Text("%.0f", m_RequestPricesDatas.prices.times.at(idx));
                }
                if (ImGui::TableNextColumn()) {  // open
                    ImGui::Text("%f", m_RequestPricesDatas.prices.opens.at(idx));
                }
                if (ImGui::TableNextColumn()) {  // close
                    ImGui::Text("%f", m_RequestPricesDatas.prices.closes.at(idx));
                }
                if (ImGui::TableNextColumn()) {  // high
                    ImGui::Text("%f", m_RequestPricesDatas.prices.highs.at(idx));
                }
                if (ImGui::TableNextColumn()) {  // low
                    ImGui::Text("%f", m_RequestPricesDatas.prices.lows.at(idx));
                }
                if (ImGui::TableNextColumn()) {  // volume
                    ImGui::Text("%.0f", m_RequestPricesDatas.prices.volumes.at(idx));
                }
            }
        }
        m_DataListClipper.End();
        ImGui::EndTable();
    }
    ImGui::PopID();
}
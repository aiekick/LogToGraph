/*
Copyright 2022-2023 Stephane Cuillerdier (aka aiekick)

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include "GraphListPane.h"
#include <project/ProjectFile.h>
#include <cinttypes>  // printf zu
#include <panes/LogPane.h>
#include <panes/CodePane.h>

#include <models/log/LogEngine.h>
#include <models/log/SignalSerie.h>
#include <models/log/SignalTick.h>
#include <models/graphs/GraphView.h>

#define GRAPHS_HEIGHT 35.0f
#define GRAPHS_COLOR_MAP ImPlotColormap_Cool

///////////////////////////////////////////////////////////////////////////////////
//// IMGUI PANE ///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////

void GraphListPane::Clear() {
    m_CategorizedSignalSeries.clear();
    m_FilteredSignalSeries.clear();
}

bool GraphListPane::Init() {
    return true;
}

void GraphListPane::Unit() {}

bool GraphListPane::DrawPanes(const uint32_t& /*vCurrentFrame*/, bool* vOpened, ImGuiContext* vContextPtr, void* /*vUserDatas*/) {
    ImGui::SetCurrentContext(vContextPtr);
    bool change = false;
    if (vOpened != nullptr && *vOpened) {
        static ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_MenuBar;
        if (ImGui::Begin(GetName().c_str(), vOpened, flags)) {
#ifdef USE_DECORATIONS_FOR_RESIZE_CHILD_WINDOWS
            auto win = ImGui::GetCurrentWindowRead();
            if (win->Viewport->Idx != 0)
                flags |= ImGuiWindowFlags_NoResize;
            else
                flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_MenuBar;
#endif
            if (ProjectFile::Instance()->IsProjectLoaded()) {
                DrawTree();
            }
        }

        ImGui::End();
    }
    return change;
}

void GraphListPane::UpdateDB() {
    m_CategorizedSignalSeries.clear();

    for (auto& item_cat : LogEngine::Instance()->GetSignalSeries()) {
        for (auto& item_name : item_cat.second) {
            m_CategorizedSignalSeries[item_cat.first].push_back(item_name.second);
        }
    }

    PrepareLog(ProjectFile::Instance()->m_AllGraphSignalsSearchString);
}

void GraphListPane::DisplayItem(const int& vIdx, const SignalSerieWeak& vDatasSerie) {
    if (!vDatasSerie.expired()) {
        ImGui::PushID(ImGui::IncPUSHID());

        auto datas_ptr = vDatasSerie.lock();
        if (datas_ptr) {
            ImGui::PushID(vIdx);
            ImGui::TableSetColumnIndex(0);
            if (ImGui::Selectable(datas_ptr->category.c_str(), &datas_ptr->show, ImGuiSelectableFlags_SpanAllColumns, ImVec2(0, GRAPHS_HEIGHT))) {
                LogEngine::Instance()->ShowHideSignal(datas_ptr->category, datas_ptr->name, datas_ptr->show);
                if (ProjectFile::Instance()->m_CollapseLogSelection) {
                    LogPane::Instance()->PrepareLog();
                }
                ProjectFile::Instance()->SetProjectChange();
            }

            ImGui::TableSetColumnIndex(1);
            if (ImGui::Selectable(datas_ptr->name.c_str(), &datas_ptr->show, ImGuiSelectableFlags_SpanAllColumns, ImVec2(0, GRAPHS_HEIGHT))) {
                LogEngine::Instance()->ShowHideSignal(datas_ptr->category, datas_ptr->name, datas_ptr->show);
                if (ProjectFile::Instance()->m_CollapseLogSelection) {
                    LogPane::Instance()->PrepareLog();
                }
                ProjectFile::Instance()->SetProjectChange();
            }

            ImGui::TableSetColumnIndex(2);
            const auto& col_u32 = datas_ptr->show ? datas_ptr->color_u32 : ImPlot::GetColormapColorU32(vIdx, GRAPHS_COLOR_MAP);
            ImDrawList* draw_list = ImPlot::GetPlotDrawList();
            ImPlot::PushStyleVar(ImPlotStyleVar_PlotPadding, ImVec2(0, 0));
            const auto& time_range = LogEngine::Instance()->GetTicksTimeSerieRange();
            if (ImPlot::BeginPlot(datas_ptr->name.c_str(), ImVec2(-1, GRAPHS_HEIGHT), ImPlotFlags_CanvasOnly | ImPlotFlags_NoChild)) {
                ImPlot::SetupAxes(0, 0, ImPlotAxisFlags_NoDecorations, ImPlotAxisFlags_NoDecorations);
                auto& datas_ptr_range_value = datas_ptr->range_value;
                double y_offset = (datas_ptr_range_value.y - datas_ptr_range_value.x) * 0.1;
                if (ez::isEqual(y_offset, 0.0)) {
                    y_offset = 0.5;
                }

                ImPlot::SetupAxisLimits(ImAxis_X1, time_range.x, time_range.y, ImPlotCond_Always);
                ImPlot::SetupAxisLimits(ImAxis_Y1, datas_ptr_range_value.x - y_offset, datas_ptr_range_value.y + y_offset, ImPlotCond_Always);
                if (ImPlot::BeginItem(datas_ptr->name.c_str())) {
                    ImPlot::GetCurrentItem()->Color = col_u32;
                    auto& datas_ptr_datas_values = datas_ptr->datas_values;
                    if (datas_ptr_datas_values.size() > 0U) {
                        ImVec2 last_value_pos, value_pos;
                        auto _data_ptr_0 = datas_ptr_datas_values.at(0U).lock();
                        if (_data_ptr_0) {
                            double last_time = _data_ptr_0->time_epoch, current_time;
                            double current_value = 0.0;
                            last_value_pos = ImPlot::PlotToPixels(last_time, _data_ptr_0->value);
                            for (size_t i = 1U; i < datas_ptr_datas_values.size(); ++i) {
                                auto _data_ptr_i = datas_ptr_datas_values.at(i).lock();
                                if (_data_ptr_i) {
                                    current_time = _data_ptr_i->time_epoch;
                                    current_value = _data_ptr_i->value;
                                    value_pos = ImPlot::PlotToPixels(current_time, current_value);
                                    ImPlot::FitPoint(ImPlotPoint(current_time, current_value));
                                    draw_list->AddLine(last_value_pos, ImVec2(value_pos.x, last_value_pos.y), col_u32, 2.0f);
                                    draw_list->AddLine(ImVec2(value_pos.x, last_value_pos.y), value_pos, col_u32, 2.0f);
                                    last_value_pos = value_pos;
                                }
                            }
                        }
                    }

                    ImPlot::EndItem();
                }

                ImPlot::EndPlot();
            }
            ImPlot::PopStyleVar();
            ImGui::PopID();
        }

        ImGui::PopID();
    }
}

void GraphListPane::DrawTree() {
    auto& search_string = ProjectFile::Instance()->m_AllGraphSignalsSearchString;

    if (ImGui::BeginMenuBar()) {
        ImGui::Text("%s", "Search : ");

        snprintf(m_search_buffer, 1024, "%s", search_string.c_str());
        if (ImGui::ContrastedButton("R##GraphListPane_SearchDrawTree")) {
            search_string.clear();
            m_search_buffer[0] = '\0';
            PrepareLog(search_string);
        }

        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
        if (ImGui::InputText("##GraphListPane_Search", m_search_buffer, 1024)) {
            search_string = ez::str::toLower(m_search_buffer);
            PrepareLog(search_string);
        }
        ImGui::PopItemWidth();

        ImGui::EndMenuBar();
    }

    static ImGuiTableFlags flags = ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_RowBg | ImGuiTableFlags_Hideable | ImGuiTableFlags_ScrollY |
        ImGuiTableFlags_NoHostExtendY | ImGuiTableFlags_Resizable;

    // if first frame is not built
    if (m_FilteredSignalSeries.empty()) {
        PrepareLog(search_string);
    }

    auto listViewID = ImGui::GetID("##GraphListPane_DrawFiltteredTable");
    if (ImGui::BeginTableEx("##GraphListPane_DrawFiltteredTable", listViewID, 3, flags))  //-V112
    {
        ImGui::TableSetupScrollFreeze(0, 1);  // Make header always visible
        ImGui::TableSetupColumn("Category", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("Graph", ImGuiTableColumnFlags_WidthStretch);

        ImGui::TableHeadersRow();

        m_VirtualClipper.Begin((int)m_FilteredSignalSeries.size(), GRAPHS_HEIGHT);
        while (m_VirtualClipper.Step()) {
            for (int i = m_VirtualClipper.DisplayStart; i < m_VirtualClipper.DisplayEnd; ++i) {
                if (i < 0)
                    continue;

                const auto infos_ptr = m_FilteredSignalSeries.at((size_t)i).lock();
                if (infos_ptr) {
                    ImGui::TableNextRow();

                    DisplayItem(i, infos_ptr);
                }
            }
        }
        m_VirtualClipper.End();

        ImGui::EndTable();
    }
}

void GraphListPane::PrepareLog(const std::string& vSearchString) {
    const bool& is_their_some_search = !vSearchString.empty();

    m_FilteredSignalSeries.clear();

    for (auto& item_cat : m_CategorizedSignalSeries) {
        for (auto& item_name : item_cat.second) {
            auto signal_ptr = item_name.lock();
            if (signal_ptr) {
                if (is_their_some_search && signal_ptr->low_case_name_for_search.find(vSearchString) == std::string::npos) {
                    continue;
                }

                m_FilteredSignalSeries.push_back(item_name);
            }
        }
    }
}

void GraphListPane::HideAllGraphs() {
    bool _one_at_least = false;

    for (auto& item_cat : LogEngine::Instance()->GetSignalSeries()) {
        for (auto& item_name : item_cat.second) {
            if (item_name.second) {
                if (item_name.second->show) {
                    _one_at_least = true;
                }

                LogEngine::Instance()->ShowHideSignal(item_name.second->category, item_name.second->name, false);
            }
        }
    }

    if (_one_at_least) {
        GraphView::Instance()->Clear();
        ProjectFile::Instance()->SetProjectChange();
    }
}

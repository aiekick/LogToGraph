// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

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

#include "GraphView.h"

#include <models/log/LogEngine.h>
#include <models/log/SignalSerie.h>
#include <models/log/SignalTick.h>
#include <models/log/SignalTag.h>

#include <Project/ProjectFile.h>

#include <models/graphs/GraphGroup.h>

#include <panes/LogPane.h>

#include <models/graphs/GraphAnnotation.h>
#include <models/graphs/GraphAnnotationModel.h>

#define DRAG_LINE_LOG_HOVERED_TIME 0
#define DRAG_LINE_FIRST_DIFF_MARK 1
#define DRAG_LINE_SECOND_DIFF_MARK 2
#define DRAG_LINE_MOUSE_HOVERED_TIME 2

static GraphColor s_DefaultGraphColors;

// todo : to put in a common file
// https://www.shadertoy.com/view/ld3fzf
ez::fvec4 GraphView::GetRainBow(const int32_t& vIdx, const int32_t& vCount) {
    float r = (float)(vIdx + 1U) / (float)vCount;
    auto c = ez::cos(ez::fvec4(0.0f, 23.0f, 21.0f, 1.0f) + r * 6.3f) * 0.5f + 0.5f;
    c.w = 0.75f;
    return c;
}

GraphView::GraphView() {
    Clear();
}

void GraphView::Clear() {
    m_GraphGroups.clear();
    m_GraphGroups.push_back(GraphGroup::Create());  // first group : default group
    m_GraphGroups.push_back(GraphGroup::Create());  // last group
    m_Range_Value = SignalValueRange(0.5, -0.5) * DBL_MAX;
}

void GraphView::AddSerieToGroup(const SignalSerieWeak& vSignalSerie, const GraphGroupPtr& vToGroupPtr) {
    auto ptr = vSignalSerie.lock();
    if (ptr && vToGroupPtr) {
        // if this is the last, the last group will become a new group
        if (vToGroupPtr == m_GraphGroups.back()) {
            auto _ptr = GraphGroup::Create();
            if (_ptr) {
                m_GraphGroups.push_back(_ptr);
            }
        }

        ptr->graph_groupd_ptr = vToGroupPtr;

        vToGroupPtr->AddSignalSerie(vSignalSerie);

        m_Range_Value.x = ez::mini(m_Range_Value.x, vToGroupPtr->GetSignalSeriesRange().x);
        m_Range_Value.y = ez::maxi(m_Range_Value.y, vToGroupPtr->GetSignalSeriesRange().y);
    }
}

void GraphView::AddSerieToGroupID(const SignalSerieWeak& vSignalSerie, const size_t& vToGroupID) {
    auto ptr = vSignalSerie.lock();
    if (ptr) {
        while (m_GraphGroups.size() <= vToGroupID + 1)  // +1 because always one more than the current max
        {
            auto _ptr = GraphGroup::Create();
            if (_ptr) {
                m_GraphGroups.push_back(_ptr);
            }
        }

        auto group_ptr = prGetGroupAt(vToGroupID);
        if (group_ptr) {
            ptr->graph_groupd_ptr = group_ptr;

            group_ptr->AddSignalSerie(vSignalSerie);

            m_Range_Value.x = ez::mini(m_Range_Value.x, group_ptr->GetSignalSeriesRange().x);
            m_Range_Value.y = ez::maxi(m_Range_Value.y, group_ptr->GetSignalSeriesRange().y);

            ComputeGraphsCount();
        }
    }
}

void GraphView::AddSerieToDefaultGroup(const SignalSerieWeak& vSignalSerie) {
    AddSerieToGroup(vSignalSerie, m_GraphGroups.front());
}

void GraphView::RemoveSerieFromGroup(const SignalSerieWeak& vSignalSerie, const GraphGroupPtr& vFromGroupPtr) {
    if (vFromGroupPtr) {
        vFromGroupPtr->RemoveSignalSerie(vSignalSerie);

        m_Range_Value.x = ez::mini(m_Range_Value.x, vFromGroupPtr->GetSignalSeriesRange().x);
        m_Range_Value.y = ez::maxi(m_Range_Value.y, vFromGroupPtr->GetSignalSeriesRange().y);
    }
}

void GraphView::RemoveEmptyGroups() {
    if (m_GraphGroups.size() > 2U) {
        std::list<size_t> group_to_erase;

        // first pass : we will save the group id of empty groups
        size_t idx = 0U;
        for (const auto& group_ptr : m_GraphGroups) {
            if (group_ptr &&                           // valid group ptr
                group_ptr != m_GraphGroups.front() &&  // not the first group (must always be selectable, so we keep it)
                group_ptr != m_GraphGroups.back() &&   // not the last group (must always be selectable, so we keep it)
                group_ptr->GetSignalSeries().empty())  // no signals series
            {
                // we insert in front
                // like that we will erase in inverse order
                // because if we delete e index 1 before a 2
                // after the 2 is in fact the 1 and we can have an issue
                // if we delete the 2 before the 1, all is ok
                group_to_erase.push_front(idx);
            }

            ++idx;
        }

        // second pass : we will delete the found group ids
        for (const auto& group_id : group_to_erase) {
            prEraseGroupAt(group_id);
        }
    }

    ComputeGraphsCount();
}

void GraphView::MoveSerieFromGroupToGroup(const SignalSerieWeak& vSignalSerie, const GraphGroupPtr& vFromGroupPtr, const GraphGroupPtr& vToGroupPtr) {
    RemoveSerieFromGroup(vSignalSerie, vFromGroupPtr);
    AddSerieToGroup(vSignalSerie, vToGroupPtr);
    RemoveEmptyGroups();
}

size_t GraphView::GetGroupID(const GraphGroupPtr& vToGroupPtr) const {
    size_t idx = 0U;
    for (const auto& group_ptr : m_GraphGroups) {
        if (group_ptr == vToGroupPtr) {
            break;
        }

        ++idx;
    }

    return idx;
}

GraphGroupsRef GraphView::GetGraphGroups() {
    return m_GraphGroups;
}

void GraphView::DrawGraphGroupTable() {
    if (ImGui::BeginMenuBar()) {
        ImGui::MenuItem("ReColorize (Rainbow)", nullptr, &ProjectFile::Instance()->m_AutoColorize);

        ImGui::EndMenuBar();
    }

    static ImGuiTableFlags flags = ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY | ImGuiTableFlags_NoHostExtendY;

    const auto _column_count = (int32_t)m_GraphGroups.size();
    auto listViewID = ImGui::GetID("##GraphView_DrawGraphGroupTable");
    if (ImGui::BeginTableEx("##GraphView_DrawGraphGroupTable", listViewID, 3U + _column_count, flags))  //-V112
    {
        ImGui::TableSetupScrollFreeze(0, 1);  // Make header always visible
        ImGui::TableSetupColumn("Vis", ImGuiTableColumnFlags_WidthFixed, -1);
        ImGui::TableSetupColumn("Col", ImGuiTableColumnFlags_WidthFixed, -1);
        ImGui::TableSetupColumn("Signal", ImGuiTableColumnFlags_WidthStretch, -1);
        ImGui::TableSetupColumn("GDef", ImGuiTableColumnFlags_WidthFixed, -1);
        for (int32_t _col_idx = 1; _col_idx < _column_count; ++_col_idx) {
            auto str = ez::str::toStr("G%i", _col_idx - 1);
            ImGui::TableSetupColumn(str.c_str(), ImGuiTableColumnFlags_WidthFixed, -1);
        }
        ImGui::TableHeadersRow();

        auto visible_count = LogEngine::Instance()->GetVisibleCount();

        // var for move singla from group ptr to group idx
        SignalSeriePtr move_signal_ptr = nullptr;
        GraphGroupPtr move_to_group_ptr = nullptr;

        int32_t visible_idx = 0;
        for (auto& item_cat : LogEngine::Instance()->GetSignalSeries()) {
            for (auto& item_name : item_cat.second) {
                auto datas_ptr = item_name.second;
                if (datas_ptr) {
                    if (datas_ptr->show) {
                        if (ProjectFile::Instance()->m_AutoColorize) {
                            datas_ptr->color_u32 = ImGui::GetColorU32(GetRainBow(visible_idx, visible_count));
                            datas_ptr->color_v4 = ImGui::ColorConvertU32ToFloat4(datas_ptr->color_u32);
                        }

                        ImGui::PushID(ImGui::IncPUSHID());
                        {
                            ImGui::TableNextRow();

                            ImGui::TableSetColumnIndex(0);
                            if (ImGui::CheckBoxBoolDefault("##vis", &datas_ptr->show_hide_temporary, true)) {
                                GraphView::Instance()->ComputeGraphsCount();
                            }

                            ImGui::TableSetColumnIndex(1);
                            if (ImGui::ColorEdit3("##colors", &datas_ptr->color_v4.x, ImGuiColorEditFlags_NoInputs)) {
                                datas_ptr->color_u32 = ImGui::GetColorU32(datas_ptr->color_v4);
                                ProjectFile::Instance()->m_AutoColorize = false;
                                ProjectFile::Instance()->SetProjectChange();
                            }

                            ImGui::TableSetColumnIndex(2);
                            if (ImGui::Selectable(datas_ptr->name.c_str(), false, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowItemOverlap)) {
                                datas_ptr->show = !datas_ptr->show;

                                LogEngine::Instance()->ShowHideSignal(datas_ptr->category, datas_ptr->name, datas_ptr->show);

                                if (ProjectFile::Instance()->m_CollapseLogSelection) {
                                    LogPane::Instance()->PrepareLog();
                                }

                                ProjectFile::Instance()->SetProjectChange();
                            }

                            int32_t _col_idx = 0;
                            for (auto& group_ptr : m_GraphGroups) {
                                ImGui::TableSetColumnIndex(3 + _col_idx);
                                ImGui::PushID(ImGui::IncPUSHID());
                                {
                                    if (ImGui::RadioButtonLabeled(ImGui::GetFrameHeight(), "x", datas_ptr->graph_groupd_ptr == group_ptr, false)) {
                                        move_signal_ptr = datas_ptr;
                                        move_to_group_ptr = group_ptr;
                                    }
                                }
                                ImGui::PopID();

                                ++_col_idx;
                            }
                        }
                        ImGui::PopID();

                        ++visible_idx;
                    }
                }
            }
        }

        ImGui::EndTable();

        // apply the move
        if (move_signal_ptr && move_to_group_ptr) {
            MoveSerieFromGroupToGroup(move_signal_ptr, move_signal_ptr->graph_groupd_ptr, move_to_group_ptr);
            ProjectFile::Instance()->SetProjectChange();

            move_signal_ptr = nullptr;
            move_to_group_ptr = nullptr;
        }
    }
}

void GraphView::DrawMenuBar() {
    if (ImGui::BeginMenu("Settings")) {
        ImGui::MenuItem("Synchronize Graphs", nullptr, &ProjectFile::Instance()->m_SyncGraphs);

        if (ImGui::BeginMenu("Axis Labels")) {
            if (ImGui::MenuItem(m_show_hide_x_axis ? "Show X Axis LabelsR##GraphPaneDrawPanes" : "Hide X Axis LabelsR##GraphPaneDrawPanes")) {
                m_show_hide_x_axis = !m_show_hide_x_axis;
                m_need_show_hide_x_axis = true;
            }

            if (ImGui::MenuItem(m_show_hide_y_axis ? "Show Y Axis LabelsR##GraphPaneDrawPanes" : "Hide Y Axis LabelsR##GraphPaneDrawPanes")) {
                m_show_hide_y_axis = !m_show_hide_y_axis;
                m_need_show_hide_y_axis = true;
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Colors")) {
            if (ImGui::ContrastedButton("R##ResetBarColor")) {
                ProjectFile::Instance()->m_GraphColors.graphBarColor = s_DefaultGraphColors.graphBarColor;
            }

            ImGui::SameLine();

            ImGui::ColorEdit4("Bars color##tBarColor", &ProjectFile::Instance()->m_GraphColors.graphBarColor.x, ImGuiColorEditFlags_NoInputs);

            if (ImGui::ContrastedButton("R##ResetHoveredTimeBarColor")) {
                ProjectFile::Instance()->m_GraphColors.graphHoveredTimeColor = s_DefaultGraphColors.graphHoveredTimeColor;
            }

            ImGui::SameLine();

            ImGui::ColorEdit4("Current Time color##HoveredTimeBarColor", &ProjectFile::Instance()->m_GraphColors.graphHoveredTimeColor.x, ImGuiColorEditFlags_NoInputs);

            if (ImGui::ContrastedButton("R##ResetMouseHoveredTimeBarColor")) {
                ProjectFile::Instance()->m_GraphColors.graphMouseHoveredTimeColor = s_DefaultGraphColors.graphMouseHoveredTimeColor;
            }

            ImGui::SameLine();

            ImGui::ColorEdit4(
                "Mouse Over color##MouseHoveredTimeBarColor", &ProjectFile::Instance()->m_GraphColors.graphMouseHoveredTimeColor.x, ImGuiColorEditFlags_NoInputs);

            if (ImGui::ContrastedButton("R##ResetBarFirstDiffMarkColor")) {
                ProjectFile::Instance()->m_GraphColors.graphFirstDiffMarkColor = s_DefaultGraphColors.graphFirstDiffMarkColor;
            }

            ImGui::SameLine();

            ImGui::ColorEdit4(
                "First diff mark color##ResetBarFirstDiffMarkColor", &ProjectFile::Instance()->m_GraphColors.graphFirstDiffMarkColor.x, ImGuiColorEditFlags_NoInputs);

            if (ImGui::ContrastedButton("R##ResetBarSecondDiffMarkColor")) {
                ProjectFile::Instance()->m_GraphColors.graphSecondDiffMarkColor = s_DefaultGraphColors.graphSecondDiffMarkColor;
            }

            ImGui::SameLine();

            ImGui::ColorEdit4(
                "Second diff mark color##ResetBarSecondDiffMarkColor", &ProjectFile::Instance()->m_GraphColors.graphSecondDiffMarkColor.x, ImGuiColorEditFlags_NoInputs);

            ImGui::EndMenu();
        }

        ImGui::EndMenu();
    }

    ImGui::Text("Diff (?)");
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Diff :\npress key 'f' for first tick\npress key 's' for second tick\npress key 'r' for reset diff marks");
    }
}

GraphGroupPtr GraphView::prGetGroupAt(const size_t& vIdx) {
    if (m_GraphGroups.size() > vIdx) {
        auto it = std::next(m_GraphGroups.begin(), vIdx);
        if (it != m_GraphGroups.end()) {
            return *it;
        }
    }

    return nullptr;
}

void GraphView::prEraseGroupAt(const size_t& vIdx) {
    if (m_GraphGroups.size() > vIdx) {
        auto it = std::next(m_GraphGroups.begin(), vIdx);
        if (it != m_GraphGroups.end()) {
            m_GraphGroups.erase(it);
        }
    }
}

bool GraphView::prBeginPlot(const std::string& vLabel, ez::dvec2 vRangeValue, const ImVec2& vSize, const bool& vFirstGraph) const {
    const auto& time_range = LogEngine::Instance()->GetTicksTimeSerieRange();
    if (ImPlot::BeginPlot(vLabel.c_str(), vSize, ImPlotFlags_NoChild | ImPlotFlags_NoTitle)) {
        if (m_need_show_hide_x_axis) {
            ImPlotPlot& plot = *GImPlot->CurrentPlot;
            ImPlotAxis& axis = plot.Axes[ImAxis_X1];
            ImFlipFlag(axis.Flags, ImPlotAxisFlags_NoTickLabels);
        }

        if (m_need_show_hide_y_axis) {
            ImPlotPlot& plot = *GImPlot->CurrentPlot;
            ImPlotAxis& axis = plot.Axes[ImAxis_Y1];
            ImFlipFlag(axis.Flags, ImPlotAxisFlags_NoTickLabels);
        }

        if (vFirstGraph) {
            ImPlot::SetupAxes(nullptr, nullptr, ImPlotAxisFlags_Opposite, ImPlotAxisFlags_Lock);
        } else {
            ImPlot::SetupAxes(nullptr, nullptr, ImPlotAxisFlags_NoTickLabels, ImPlotAxisFlags_Lock);
        }

        double y_offset = (vRangeValue.y - vRangeValue.x) * 0.1;
        if (ez::isEqual(y_offset, 0.0)) {
            y_offset = 0.5;
        }

        ImPlot::SetupAxisLimits(ImAxis_X1, time_range.x, time_range.y);
        ImPlot::SetupAxisLimits(ImAxis_Y1, vRangeValue.x - y_offset, vRangeValue.y + y_offset, ImPlotCond_Always);

        ImPlot::SetupAxisScale(ImAxis_X1, ImPlotScale_Time);

        ImPlot::SetupAxisLimitsConstraints(ImAxis_X1, time_range.x, time_range.y);
        ImPlot::SetupAxisLimitsConstraints(ImAxis_Y1, vRangeValue.x - y_offset, vRangeValue.y + y_offset);
        ImPlot::SetupAxisFormat(ImAxis_Y1, "%.3f");

        if (ProjectFile::Instance()->m_SyncGraphs) {
            ImPlot::SetupAxisLinks(ImAxis_X1, &ProjectFile::Instance()->m_SyncGraphsLimits.X.Min, &ProjectFile::Instance()->m_SyncGraphsLimits.X.Max);
        }

        if (ImPlot::IsPlotHovered()) {
            LogEngine::Instance()->SetHoveredTime(ImPlot::GetPlotMousePos().x);

            // first mark
            if (ImGui::IsKeyPressed(ImGuiKey_F)) {
                LogEngine::Instance()->SetFirstDiffMark(ImPlot::GetPlotMousePos().x);
            }

            // second mark
            if (ImGui::IsKeyPressed(ImGuiKey_S)) {
                LogEngine::Instance()->SetSecondDiffMark(ImPlot::GetPlotMousePos().x);
            }

            // second mark
            if (ImGui::IsKeyPressed(ImGuiKey_R)) {
                LogEngine::Instance()->SetFirstDiffMark(0.0);
                LogEngine::Instance()->SetSecondDiffMark(0.0);
            }
        }

        return true;
    }

    return false;
}

// from DragLineX
static bool ImPLotHoveredLineX(int n_id, double* value, const ImVec4& col, float thickness, ImPlotDragToolFlags flags) {
    // ImGui::PushID("#IMPLOT_DRAG_LINE_X");
    ImPlotContext& gp = *GImPlot;
    IM_ASSERT_USER_ERROR(gp.CurrentPlot != NULL, "DragLineX() needs to be called between BeginPlot() and EndPlot()!");
    ImPlot::SetupLock();

    if (!ImHasFlag(flags, ImPlotDragToolFlags_NoFit) && ImPlot::FitThisFrame()) {
        ImPlot::FitPointX(*value);
    }

    const bool input = !ImHasFlag(flags, ImPlotDragToolFlags_NoInputs);
    const bool show_curs = !ImHasFlag(flags, ImPlotDragToolFlags_NoCursors);
    // const bool no_delay = !ImHasFlag(flags, ImPlotDragToolFlags_Delayed);
    static const float DRAG_GRAB_HALF_SIZE = 4.0f;
    const float grab_half_size = ImMax(DRAG_GRAB_HALF_SIZE, thickness / 2);
    float yt = gp.CurrentPlot->PlotRect.Min.y;
    float yb = gp.CurrentPlot->PlotRect.Max.y;
    float x = IM_ROUND(ImPlot::PlotToPixels(*value, 0, IMPLOT_AUTO, IMPLOT_AUTO).x);
    const ImGuiID id = ImGui::GetCurrentWindow()->GetID(n_id);
    ImRect rect(x - grab_half_size, yt, x + grab_half_size, yb);
    bool hovered = false, held = false;

    ImGui::KeepAliveID(id);
    if (input)
        ImGui::ButtonBehavior(rect, id, &hovered, &held);

    if ((hovered || held) && show_curs)
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);

    float len = gp.Style.MajorTickLen.x;
    ImVec4 color = ImPlot::IsColorAuto(col) ? ImGui::GetStyleColorVec4(ImGuiCol_Text) : col;
    ImU32 col32 = ImGui::ColorConvertFloat4ToU32(color);

    ImPlot::PushPlotClipRect();
    ImDrawList& DrawList = *ImPlot::GetPlotDrawList();
    DrawList.AddLine(ImVec2(x, yt), ImVec2(x, yb), col32, thickness);
    DrawList.AddLine(ImVec2(x, yt), ImVec2(x, yt + len), col32, 3 * thickness);
    DrawList.AddLine(ImVec2(x, yb), ImVec2(x, yb - len), col32, 3 * thickness);
    ImPlot::PopPlotClipRect();

    // ImGui::PopID();
    return hovered;
}

void GraphView::prEndPlot(const bool& vFirstGraph) {
    // draw diff first marks
    auto first_mark = ProjectFile::Instance()->m_DiffFirstMark;
    if (first_mark > 0.0) {
        if (ImPlot::DragLineX(
                DRAG_LINE_FIRST_DIFF_MARK, &ProjectFile::Instance()->m_DiffFirstMark, ProjectFile::Instance()->m_GraphColors.graphFirstDiffMarkColor, 1.5f)) {
            LogEngine::Instance()->SetFirstDiffMark(ProjectFile::Instance()->m_DiffFirstMark);
        }

        ImPlot::TagX(ProjectFile::Instance()->m_DiffFirstMark, ProjectFile::Instance()->m_GraphColors.graphFirstDiffMarkColor, "%s", "|>");
    }

    // draw diff second marks
    auto second_mark = ProjectFile::Instance()->m_DiffSecondMark;
    if (second_mark > 0.0) {
        if (ImPlot::DragLineX(
                DRAG_LINE_SECOND_DIFF_MARK, &ProjectFile::Instance()->m_DiffSecondMark, ProjectFile::Instance()->m_GraphColors.graphSecondDiffMarkColor, 1.5f)) {
            LogEngine::Instance()->SetSecondDiffMark(ProjectFile::Instance()->m_DiffSecondMark);
        }

        ImPlot::TagX(ProjectFile::Instance()->m_DiffSecondMark, ProjectFile::Instance()->m_GraphColors.graphSecondDiffMarkColor, "%s", "<|");
    }

    auto hovered_time = LogEngine::Instance()->GetHoveredTime();
    ImPlot::DragLineX(DRAG_LINE_LOG_HOVERED_TIME,
                      &hovered_time,
                      ProjectFile::Instance()->m_GraphColors.graphMouseHoveredTimeColor,
                      1.5f,
                      ImPlotDragToolFlags_NoInputs | ImPlotDragToolFlags_NoCursors);

    for (const auto& tag_ptr : LogEngine::Instance()->GetSignalTags()) {
        if (tag_ptr) {
            auto _time_epoch = tag_ptr->time_epoch;
            if (ImPLotHoveredLineX((int)(uintptr_t)tag_ptr.get(), &_time_epoch, tag_ptr->color, 1.5f, 0)) {
                ImGui::SetTooltip("%s :\n%s", tag_ptr->name.c_str(), tag_ptr->help.c_str());
            }

            if (vFirstGraph) {
                ImPlot::TagX(tag_ptr->time_epoch, tag_ptr->color, "%s", tag_ptr->name.c_str());
            }
        }
    }

    ImPlot::EndPlot();
}

void GraphView::prDrawSignalGraph_ImPlot(const SignalSerieWeak& vSignalSerie, const ImVec2& vSize, const bool& vFirstGraph) {
    auto datas_ptr = vSignalSerie.lock();
    if (datas_ptr && datas_ptr->show_hide_temporary) {
        ImDrawList* draw_list = ImPlot::GetPlotDrawList();
        if (!draw_list)
            return;

        ImVec2 last_value_pos, value_pos;

        const auto& isp = ImGui::GetStyle().ItemSpacing;
        const auto& fpa = ImGui::GetStyle().FramePadding;
        const auto& spacing_L = fpa.x;
        const auto& spacing_U = isp.y;
        const auto& spacing_R = isp.x + fpa.x;
        const auto& spacing_D = isp.y;
        ez::dvec2 projected_point;
        const auto& _CurveRadiusDetection = ProjectFile::Instance()->m_CurveRadiusDetection;
        const auto& _GraphMouseHoveredTimeColor = ImGui::GetColorU32(ProjectFile::Instance()->m_GraphColors.graphMouseHoveredTimeColor);
        const auto& _SelectedCurveDisplayThickNess = ProjectFile::Instance()->m_SelectedCurveDisplayThickNess;
        const auto& _DefaultCurveDisplayThickNess = ProjectFile::Instance()->m_DefaultCurveDisplayThickNess;

        ImGui::PushID(ImGui::IncPUSHID());

        const double& hovered_time = LogEngine::Instance()->GetHoveredTime();
        bool _already_drawn = false;

        std::string _human_readbale_elapsed_time;
        SignalSeriePtr _current_hovered_serie = nullptr;

        const auto& name_str = datas_ptr->category + " / " + datas_ptr->name;
        if (prBeginPlot(name_str, datas_ptr->range_value, vSize, vFirstGraph)) {
            if (ImPlot::BeginItem(name_str.c_str())) {
                const float thickness = (float)(datas_ptr->hovered_by_mouse ? _SelectedCurveDisplayThickNess : _DefaultCurveDisplayThickNess);

                ImPlot::GetCurrentItem()->Color = datas_ptr->color_u32;

                // render data
                const auto& _data_values = datas_ptr->datas_values;
                if (!_data_values.empty()) {
                    // float zero_y = (float)ImPlot::PlotToPixels(0.0, 0.0).y;
                    auto _data_ptr_0 = _data_values.at(0U).lock();
                    if (_data_ptr_0) {
                        double last_time = _data_ptr_0->time_epoch, current_time;
                        double last_value = _data_ptr_0->value, current_value;
                        std::string last_string = _data_ptr_0->string, current_string;
                        std::string last_status = _data_ptr_0->status, current_status;

                        ImPlotPoint last_point = ImPlotPoint(last_time, _data_ptr_0->value);
                        last_value_pos = ImPlot::PlotToPixels(last_point);

                        for (size_t i = 1U; i < _data_values.size(); ++i) {
                            auto _data_ptr_i = _data_values.at(i).lock();
                            if (_data_ptr_i) {
                                current_time = _data_ptr_i->time_epoch;
                                current_value = _data_ptr_i->value;
                                current_string = _data_ptr_i->string;
                                current_status = _data_ptr_i->status;

                                ImPlotPoint current_point = ImPlotPoint(current_time, current_value);

                                value_pos = ImPlot::PlotToPixels(current_point);

                                const bool& _is_hovered = (ImPlot::IsPlotHovered() && hovered_time >= last_time && hovered_time <= current_time);
                                const ImU32& _color = _is_hovered ? _GraphMouseHoveredTimeColor : datas_ptr->color_u32;

                                if (last_string.empty()) {
                                    ImPlot::FitPoint(ImPlotPoint(current_time, current_value));
                                    draw_list->AddLine(last_value_pos, ImVec2(value_pos.x, last_value_pos.y), _color, thickness);
                                    draw_list->AddLine(ImVec2(value_pos.x, last_value_pos.y), value_pos, _color, thickness);
                                } else {
                                    if (last_status == LogEngine::sc_START_ZONE && current_status == LogEngine::sc_END_ZONE) {
                                        ImPlot::FitPoint(ImPlotPoint(current_time, -1.0f));
                                        ImPlot::FitPoint(ImPlotPoint(current_time, 1.0f));
                                        ImVec2 last_pos = ImPlot::PlotToPixels(last_time, -1.0f);
                                        ImVec2 cur_pos = ImPlot::PlotToPixels(current_time, 1.0f);
                                        draw_list->AddRectFilled(last_pos, cur_pos, _color);
                                        if (_is_hovered) {
                                            draw_list->AddRect(last_pos, cur_pos, ImGui::GetColorU32(ImVec4(0, 0, 0, 1)), 0.0f, 0, (float)_SelectedCurveDisplayThickNess);
                                        }
                                    }
                                }

                                // current annotation creation
                                if (m_CurrentAnnotationPtr) {
                                    m_CurrentAnnotationPtr->DrawToPoint(datas_ptr, ImGui::GetMousePos());
                                }

                                if (_is_hovered) {
                                    // for avoid frame time regeneration of this slow operation
                                    // GraphAnnotation::sGetHumanReadableElapsedTime
                                    if (_current_hovered_serie != datas_ptr) {
                                        _human_readbale_elapsed_time = GraphAnnotation::sGetHumanReadableElapsedTime(current_time - last_time);
                                    }
                                    _current_hovered_serie = datas_ptr;

                                    // mouse hover curve
                                    const auto mouse_pos = ImGui::GetMousePos();
                                    const auto last_pos = ImVec2(value_pos.x, last_value_pos.y);
                                    datas_ptr->hovered_by_mouse =
                                        GraphAnnotation::sIsMouseHoverLine(mouse_pos, _CurveRadiusDetection, last_value_pos, last_pos, projected_point);
                                    if (!datas_ptr->hovered_by_mouse) {
                                        datas_ptr->hovered_by_mouse =
                                            GraphAnnotation::sIsMouseHoverLine(mouse_pos, _CurveRadiusDetection, last_pos, value_pos, projected_point);
                                    }

                                    // annotation start and end points
                                    if (ImGui::IsMouseReleased(ImGuiMouseButton_Middle) && datas_ptr->hovered_by_mouse) {
                                        if (m_CurrentAnnotationPtr) {
                                            if (m_CurrentAnnotationPtr->GetParentSignalSerie().lock() == datas_ptr) {
                                                m_CurrentAnnotationPtr->SetEndPoint(ImPlot::PixelsToPlot(projected_point));
                                                m_CurrentAnnotationPtr = nullptr;  // remove the "draw to mouse point" of the current annotation
                                            }
                                        } else {
                                            m_CurrentAnnotationPtr = GraphAnnotationModel::Instance()->NewGraphAnnotation(ImPlot::PixelsToPlot(projected_point));
                                            m_CurrentAnnotationPtr->SetSignalSerieParent(datas_ptr);
                                            datas_ptr->AddGraphAnnotation(m_CurrentAnnotationPtr);
                                        }
                                    }

                                    // draw vertical cursor
                                    auto pos = ImPlot::PlotToPixels(hovered_time, last_value);
                                    if (!_already_drawn) {
                                        draw_list->AddLine(pos - ImVec2(20.0f, 0.0f), pos + ImVec2(20.0f, 0.0f), _GraphMouseHoveredTimeColor, 1.0f);
                                        _already_drawn = true;
                                    }

                                    // a circle by signal
                                    draw_list->AddCircle(pos, 5.0f, ImGui::GetColorU32(ProjectFile::Instance()->m_GraphColors.graphHoveredTimeColor), 24, 2.0f);

                                    // draw info tooltip
                                    ImGui::BeginTooltipEx(ImGuiTooltipFlags_None, ImGuiWindowFlags_None);

                                    if (last_string.empty()) {
                                        // tofix : to refactor
                                        const auto p_min = ImGui::GetCursorScreenPos() - ImVec2(spacing_L, spacing_U);
                                        const auto p_max =
                                            ImVec2(p_min.x + ImGui::GetContentRegionAvail().x + spacing_R, p_min.y + (ImGui::GetFrameHeight() - spacing_D));
                                        ImGui::GetWindowDrawList()->AddRectFilled(p_min, p_max, _color);
                                        const bool pushed = ImGui::PushStyleColorWithContrast4(
                                            _color, ImGuiCol_Text, ImGui::CustomStyle::puContrastedTextColor, ImGui::CustomStyle::puContrastRatio);
                                        ImGui::Text("%s : %f", name_str.c_str(), last_value);
                                        if (pushed)
                                            ImGui::PopStyleColor();
                                    } else {
                                        if (last_status == LogEngine::sc_START_ZONE && current_status == LogEngine::sc_END_ZONE) {
                                            // tofix : to refactor
                                            const auto p_min = ImGui::GetCursorScreenPos() - ImVec2(spacing_L, spacing_U);
                                            const auto p_max =
                                                ImVec2(p_min.x + ImGui::GetContentRegionAvail().x + spacing_R, p_min.y + (ImGui::GetFrameHeight() - spacing_D) * 2.0f);
                                            ImGui::GetWindowDrawList()->AddRectFilled(p_min, p_max, _color);
                                            const bool pushed = ImGui::PushStyleColorWithContrast4(
                                                _color, ImGuiCol_Text, ImGui::CustomStyle::puContrastedTextColor, ImGui::CustomStyle::puContrastRatio);
                                            ImGui::Text("%s : %s\nElapsed time : %s", name_str.c_str(), last_string.c_str(), _human_readbale_elapsed_time.c_str());
                                            if (pushed)
                                                ImGui::PopStyleColor();
                                        }
                                    }

                                    ImGui::EndTooltip();
                                }

                                last_value_pos = value_pos;
                                last_time = current_time;
                                last_value = current_value;
                                last_string = current_string;
                                last_status = current_status;
                            }
                        }

                        // draw annotations
                        datas_ptr->DrawAnnotations();
                    }
                }

                ImPlot::EndItem();
            }

            if (ImPlot::IsPlotHovered()) {
                auto date_str = LogEngine::sConvertEpochToDateTimeString(hovered_time);

                // will finalize the tooltip by adding the current time of the mouse hover pos
                ImGui::BeginTooltipEx(ImGuiTooltipFlags_None, ImGuiWindowFlags_None);
                ImGui::Text("time : %f\ndate : %s", hovered_time, date_str.c_str());
                ImGui::EndTooltip();
            }

            prEndPlot(vFirstGraph);
        }

        ImGui::PopID();
    }
}

void GraphView::DrawAloneGraphs(const GraphGroupPtr& vGraphGroupPtr, const ImVec2& vSize, bool& vFirstGraph) {
    if (vGraphGroupPtr) {
        for (auto& cat : vGraphGroupPtr->GetSignalSeries()) {
            for (auto& name : cat.second) {
                auto datas_ptr = name.second.lock();
                if (datas_ptr) {
                    prDrawSignalGraph_ImPlot(datas_ptr, vSize, vFirstGraph);

                    vFirstGraph = false;
                }
            }
        }
    }
}

void GraphView::DrawGroupedGraphs(const GraphGroupPtr& vGraphGroupPtr, const ImVec2& vSize, bool& vFirstGraph) {
    if (vGraphGroupPtr) {
        ImDrawList* draw_list = ImPlot::GetPlotDrawList();
        if (!draw_list)
            return;

        if (!vGraphGroupPtr->GetSignalSeries().empty())  // if not empty
        {
            ImVec2 last_value_pos, value_pos;

            const auto& isp = ImGui::GetStyle().ItemSpacing;
            const auto& fpa = ImGui::GetStyle().FramePadding;
            const auto& spacing_L = fpa.x;
            const auto& spacing_U = isp.y;
            const auto& spacing_R = isp.x + fpa.x;
            const auto& spacing_D = isp.y;
            ez::dvec2 projected_point;
            const auto& _CurveRadiusDetection = ProjectFile::Instance()->m_CurveRadiusDetection;
            const auto& _GraphMouseHoveredTimeColor = ImGui::GetColorU32(ProjectFile::Instance()->m_GraphColors.graphMouseHoveredTimeColor);
            const auto& _SelectedCurveDisplayThickNess = ProjectFile::Instance()->m_SelectedCurveDisplayThickNess;
            const auto& _DefaultCurveDisplayThickNess = ProjectFile::Instance()->m_DefaultCurveDisplayThickNess;
            const auto& mouse_pos = ImGui::GetMousePos();

            ImGui::PushID(ImGui::IncPUSHID());

            if (prBeginPlot(vGraphGroupPtr->GetImGuiLabel(), vGraphGroupPtr->GetSignalSeriesRange(), vSize, vFirstGraph)) {
                const auto& hovered_time = LogEngine::Instance()->GetHoveredTime();
                bool _already_drawn = false;

                std::string _human_readbale_elapsed_time;
                SignalSeriePtr _current_hovered_serie = nullptr;

                float _ZoneYOffset = 0.0f;  // y offset from zone to zone each new series

                for (auto& cat : vGraphGroupPtr->GetSignalSeries()) {
                    for (auto& name : cat.second) {
                        auto datas_ptr = name.second.lock();
                        if (datas_ptr && datas_ptr->show_hide_temporary) {
                            const auto& name_str = datas_ptr->category + " / " + datas_ptr->name;
                            if (ImPlot::BeginItem(name_str.c_str())) {
                                bool _is_zone_reached = false;

                                const float thickness = (float)(datas_ptr->hovered_by_mouse ? _SelectedCurveDisplayThickNess : _DefaultCurveDisplayThickNess);

                                ImPlot::GetCurrentItem()->Color = datas_ptr->color_u32;

                                auto& _data_values = datas_ptr->datas_values;
                                if (!_data_values.empty()) {
                                    auto _data_ptr_0 = _data_values.at(0U).lock();
                                    if (_data_ptr_0) {
                                        double last_time = _data_ptr_0->time_epoch, current_time;
                                        double last_value = _data_ptr_0->value, current_value;
                                        std::string last_string = _data_ptr_0->string, current_string;
                                        std::string last_status = _data_ptr_0->status, current_status;
                                        bool _is_h_hovered = false;

                                        last_value_pos = ImPlot::PlotToPixels(last_time, _data_ptr_0->value);
                                        for (size_t i = 1U; i < _data_values.size(); ++i) {
                                            auto _data_ptr_i = _data_values.at(i).lock();
                                            if (_data_ptr_i) {
                                                current_time = _data_ptr_i->time_epoch;
                                                current_value = _data_ptr_i->value;
                                                current_string = _data_ptr_i->string;
                                                current_status = _data_ptr_i->status;
                                                value_pos = ImPlot::PlotToPixels(current_time, current_value);

                                                ImPlot::FitPoint(ImPlotPoint(current_time, current_value));

                                                const bool& _is_v_hovered = (ImPlot::IsPlotHovered() && hovered_time >= last_time && hovered_time <= current_time);
                                                ImU32 _color = datas_ptr->color_u32;

                                                if (last_string.empty()) {
                                                    ImPlot::FitPoint(ImPlotPoint(current_time, current_value));
                                                    draw_list->AddLine(last_value_pos, ImVec2(value_pos.x, last_value_pos.y), _color, thickness);
                                                    draw_list->AddLine(ImVec2(value_pos.x, last_value_pos.y), value_pos, _color, thickness);
                                                } else {
                                                    if (last_status == LogEngine::sc_START_ZONE && current_status == LogEngine::sc_END_ZONE) {
                                                        _is_zone_reached = true;
                                                        ImPlot::FitPoint(ImPlotPoint(current_time, _ZoneYOffset));
                                                        ImPlot::FitPoint(ImPlotPoint(current_time, _ZoneYOffset + 1.0f));
                                                        ImVec2 last_pos = ImPlot::PlotToPixels(last_time, _ZoneYOffset);
                                                        ImVec2 cur_pos = ImPlot::PlotToPixels(current_time, _ZoneYOffset + 1.0f);
                                                        _is_h_hovered = (ImPlot::IsPlotHovered() && mouse_pos.y <= last_pos.y && mouse_pos.y >= cur_pos.y);
                                                        _color = _is_v_hovered && _is_h_hovered ? _GraphMouseHoveredTimeColor : datas_ptr->color_u32;
                                                        draw_list->AddRectFilled(last_pos, cur_pos, _color);
                                                        draw_list->AddRect(last_pos, cur_pos, ImGui::GetColorU32(ImVec4(0, 0, 0, 1)));
                                                        if (_is_v_hovered && _is_h_hovered) {
                                                            draw_list->AddRect(last_pos,
                                                                               cur_pos,
                                                                               ImGui::GetColorU32(ImVec4(0, 0, 0, 1)),
                                                                               0.0f,
                                                                               0,
                                                                               (float)_SelectedCurveDisplayThickNess);
                                                        }
                                                    }
                                                }

                                                // current annotation creation
                                                if (m_CurrentAnnotationPtr) {
                                                    m_CurrentAnnotationPtr->DrawToPoint(datas_ptr, ImGui::GetMousePos());
                                                }

                                                // draw gizmo for mouse over tick
                                                if (_is_v_hovered) {
                                                    // for avoid frame time regeneration of this slow operation
                                                    // GraphAnnotation::sGetHumanReadableElapsedTime
                                                    if (_is_h_hovered) {
                                                        if (_current_hovered_serie != datas_ptr) {
                                                            _human_readbale_elapsed_time = GraphAnnotation::sGetHumanReadableElapsedTime(current_time - last_time);
                                                        }
                                                        _current_hovered_serie = datas_ptr;
                                                    }

                                                    // mouse hover curve
                                                    const auto last_pos = ImVec2(value_pos.x, last_value_pos.y);
                                                    datas_ptr->hovered_by_mouse =
                                                        GraphAnnotation::sIsMouseHoverLine(mouse_pos, _CurveRadiusDetection, last_value_pos, last_pos, projected_point);
                                                    if (!datas_ptr->hovered_by_mouse) {
                                                        datas_ptr->hovered_by_mouse =
                                                            GraphAnnotation::sIsMouseHoverLine(mouse_pos, _CurveRadiusDetection, last_pos, value_pos, projected_point);
                                                    }

                                                    // annotation start and end points
                                                    if (ImGui::IsMouseReleased(ImGuiMouseButton_Middle) && datas_ptr->hovered_by_mouse) {
                                                        if (m_CurrentAnnotationPtr) {
                                                            if (m_CurrentAnnotationPtr->GetParentSignalSerie().lock() == datas_ptr) {
                                                                m_CurrentAnnotationPtr->SetEndPoint(ImPlot::PixelsToPlot(projected_point));
                                                                m_CurrentAnnotationPtr = nullptr;  // remove the "draw to mouse point" of the current annotation
                                                            }
                                                        } else {
                                                            m_CurrentAnnotationPtr =
                                                                GraphAnnotationModel::Instance()->NewGraphAnnotation(ImPlot::PixelsToPlot(projected_point));
                                                            m_CurrentAnnotationPtr->SetSignalSerieParent(datas_ptr);
                                                            datas_ptr->AddGraphAnnotation(m_CurrentAnnotationPtr);
                                                        }
                                                    }

                                                    // draw vertical cursor
                                                    auto pos = ImPlot::PlotToPixels(hovered_time, last_value);
                                                    if (!_already_drawn) {
                                                        draw_list->AddLine(pos - ImVec2(20.0f, 0.0f), pos + ImVec2(20.0f, 0.0f), _GraphMouseHoveredTimeColor, 1.0f);
                                                        _already_drawn = true;
                                                    }

                                                    // a circle by signal
                                                    draw_list->AddCircle(
                                                        pos, 5.0f, ImGui::GetColorU32(ProjectFile::Instance()->m_GraphColors.graphHoveredTimeColor), 24, 2.0f);

                                                    // the first begin tootlip call open the tooltip and add a signal, the next begins will fill it with another signals
                                                    ImGui::BeginTooltipEx(ImGuiTooltipFlags_None, ImGuiWindowFlags_None);

                                                    if (last_string.empty()) {
                                                        // tofix : to refactor
                                                        const auto p_min = ImGui::GetCursorScreenPos() - ImVec2(spacing_L, spacing_U);
                                                        const auto p_max = ImVec2(p_min.x + ImGui::GetContentRegionAvail().x + spacing_R,
                                                                                  p_min.y + (ImGui::GetFrameHeight() - spacing_D));
                                                        ImGui::GetWindowDrawList()->AddRectFilled(p_min, p_max, _color);
                                                        const bool pushed = ImGui::PushStyleColorWithContrast4(
                                                            _color, ImGuiCol_Text, ImGui::CustomStyle::puContrastedTextColor, ImGui::CustomStyle::puContrastRatio);
                                                        ImGui::Text("%s : %f", name_str.c_str(), last_value);
                                                        if (pushed)
                                                            ImGui::PopStyleColor();
                                                    } else {
                                                        if (last_status == LogEngine::sc_START_ZONE && current_status == LogEngine::sc_END_ZONE) {
                                                            if (_current_hovered_serie == datas_ptr) {
                                                                // tofix : to refactor
                                                                const auto p_min = ImGui::GetCursorScreenPos() - ImVec2(spacing_L, spacing_U);
                                                                const auto p_max = ImVec2(p_min.x + ImGui::GetContentRegionAvail().x + spacing_R,
                                                                                          p_min.y + (ImGui::GetFrameHeight() - spacing_D) * 2.0f);
                                                                ImGui::GetWindowDrawList()->AddRectFilled(p_min, p_max, _color);
                                                                const bool pushed = ImGui::PushStyleColorWithContrast4(_color,
                                                                                                                       ImGuiCol_Text,
                                                                                                                       ImGui::CustomStyle::puContrastedTextColor,
                                                                                                                       ImGui::CustomStyle::puContrastRatio);
                                                                ImGui::Text("%s : %s\nElapsed time : %s",
                                                                            name_str.c_str(),
                                                                            last_string.c_str(),
                                                                            _human_readbale_elapsed_time.c_str());
                                                                if (pushed)
                                                                    ImGui::PopStyleColor();
                                                            } else {
                                                                // tofix : to refactor
                                                                const auto p_min = ImGui::GetCursorScreenPos() - ImVec2(spacing_L, spacing_U);
                                                                const auto p_max = ImVec2(p_min.x + ImGui::GetContentRegionAvail().x + spacing_R,
                                                                                          p_min.y + (ImGui::GetFrameHeight() - spacing_D));
                                                                ImGui::GetWindowDrawList()->AddRectFilled(p_min, p_max, _color);
                                                                const bool pushed = ImGui::PushStyleColorWithContrast4(_color,
                                                                                                                       ImGuiCol_Text,
                                                                                                                       ImGui::CustomStyle::puContrastedTextColor,
                                                                                                                       ImGui::CustomStyle::puContrastRatio);
                                                                ImGui::Text("%s : %s", name_str.c_str(), last_string.c_str());
                                                                if (pushed)
                                                                    ImGui::PopStyleColor();
                                                            }
                                                        }
                                                    }

                                                    ImGui::EndTooltip();
                                                }

                                                last_value_pos = value_pos;
                                                last_time = current_time;
                                                last_value = current_value;
                                                last_string = current_string;
                                                last_status = current_status;
                                            }
                                        }

                                        // draw annotations
                                        datas_ptr->DrawAnnotations();
                                    }
                                }

                                ImPlot::EndItem();

                                if (_is_zone_reached) {
                                    _ZoneYOffset += 1.0f;
                                }
                            }
                        }
                    }
                }

                if (ImPlot::IsPlotHovered()) {
                    ImPlotPoint plotHoveredMouse = ImPlot::GetPlotMousePos();
                    auto date_str = LogEngine::sConvertEpochToDateTimeString(plotHoveredMouse.x);

                    // will finalize the tooltip by adding the current time of the mouse hover pos
                    ImGui::BeginTooltipEx(ImGuiTooltipFlags_None, ImGuiWindowFlags_None);
                    ImGui::Text("time : %f\ndate : %s", plotHoveredMouse.x, date_str.c_str());
                    ImGui::EndTooltip();
                }

                prEndPlot(vFirstGraph);
            }

            ImGui::PopID();

            vFirstGraph = false;
        }
    }
}

void GraphView::ComputeGraphsCount() {
    m_GraphsCount = 0;
    for (auto ggit = m_GraphGroups.begin(); ggit != m_GraphGroups.end(); ++ggit) {
        auto ptr = *ggit;
        if (ptr) {
            if (ggit == m_GraphGroups.begin()) {
                for (auto& item_cat : ptr->GetSignalSeries()) {
                    for (auto& item_name : item_cat.second) {
                        auto datas_ptr = item_name.second.lock();
                        if (datas_ptr && datas_ptr->show_hide_temporary) {
                            ++m_GraphsCount;
                        }
                    }
                }
            } else if (!ptr->GetSignalSeries().empty()) {
                ++m_GraphsCount;
            }
        }
    }
}

int32_t GraphView::GetGraphCount() const {
    return m_GraphsCount;
}

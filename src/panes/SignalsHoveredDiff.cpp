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

#include "SignalsHoveredDiff.h"
#include <project/ProjectFile.h>
#include <cinttypes>  // printf zu
#include <panes/CodePane.h>

#include <models/log/LogEngine.h>
#include <models/log/SignalSerie.h>
#include <models/log/SignalTick.h>

static GraphColor s_DefaultGraphColors;

///////////////////////////////////////////////////////////////////////////////////
//// IMGUI PANE ///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////

void SignalsHoveredDiff::Clear() {
    m_PreviewTicks.clear();
}

bool SignalsHoveredDiff::Init() {
    return true;
}

void SignalsHoveredDiff::Unit() {}

bool SignalsHoveredDiff::DrawPanes(const uint32_t& /*vCurrentFrame*/, bool* vOpened, ImGuiContext* vContextPtr, void* /*vUserDatas*/) {
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
                DrawTable();
            }
        }

        ImGui::End();
    }
    return change;
}

void SignalsHoveredDiff::CheckItem(const SignalTickPtr& vSignalTick) {
    if (vSignalTick && ImGui::IsItemHovered()) {
        if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
            LogEngine::Instance()->ShowHideSignal(vSignalTick->category, vSignalTick->name);
            LogEngine::Instance()->UpdateVisibleSignalsColoring();
            ProjectFile::Instance()->SetProjectChange();
        }
    }
}

void SignalsHoveredDiff::DrawTable() {
    auto win = ImGui::GetCurrentWindowRead();
    if (win) {
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("Graph mark Colors")) {
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

                ImGui::ColorEdit4("Second diff mark color##ResetBarSecondDiffMarkColor",
                                  &ProjectFile::Instance()->m_GraphColors.graphSecondDiffMarkColor.x,
                                  ImGuiColorEditFlags_NoInputs);

                ImGui::EndMenu();
            }

            ImGui::Text("Diff (?)");
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Diff in graph :\npress key 'f' for first tick\npress key 's' for second tick\npress key 'r' for reset diff marks");
            }

            ImGui::EndMenuBar();
        }

        const auto& signals_count = LogEngine::Instance()->GetDiffResultTicks().size();
        if (signals_count) {
            static ImGuiTableFlags flags = ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_RowBg | ImGuiTableFlags_Hideable | ImGuiTableFlags_ScrollY |
                ImGuiTableFlags_NoHostExtendY | ImGuiTableFlags_Resizable;

            auto listViewID = ImGui::GetID("##SignalsHoveredDiff_DrawTable");
            if (ImGui::BeginTableEx("##SignalsHoveredDiff_DrawTable", listViewID, 4, flags))  //-V112
            {
                ImGui::TableSetupScrollFreeze(0, 1);  // Make header always visible
                ImGui::TableSetupColumn("Cat", ImGuiTableColumnFlags_WidthFixed);
                ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("First Value", ImGuiTableColumnFlags_WidthFixed);
                ImGui::TableSetupColumn("Second Value", ImGuiTableColumnFlags_WidthFixed);

                ImGui::TableHeadersRow();

                int32_t count_color_push = 0;
                ImU32 color = 0U;
                bool selected = false;
                m_VirtualClipper.Begin((int)signals_count, ImGui::GetTextLineHeightWithSpacing());
                while (m_VirtualClipper.Step()) {
                    for (int i = m_VirtualClipper.DisplayStart; i < m_VirtualClipper.DisplayEnd; ++i) {
                        if (i < 0)
                            continue;

                        const auto& diff_item = LogEngine::Instance()->GetDiffResultTicks().at((size_t)i);
                        const auto& diff_first_mark_ptr = diff_item.first.lock();
                        const auto& diff_second_mark_ptr = diff_item.second.lock();
                        if (diff_first_mark_ptr && diff_second_mark_ptr) {
                            ImGui::TableNextRow();

                            selected = LogEngine::Instance()->isSignalShown(diff_first_mark_ptr->category, diff_first_mark_ptr->name, &color);
                            if (selected && color) {
                                ImGui::PushStyleColor(ImGuiCol_Header, (ImU32)color);
                                ImGui::PushStyleColor(ImGuiCol_HeaderActive, (ImU32)color);
                                ImGui::PushStyleColor(ImGuiCol_HeaderHovered, (ImU32)color);
                                count_color_push = 3;
                                if (ImGui::PushStyleColorWithContrast1(
                                        ImGuiCol_Header, ImGuiCol_Text, ImGui::CustomStyle::puContrastedTextColor, ImGui::CustomStyle::puContrastRatio)) {
                                    count_color_push = 4;
                                }
                            } else {
                                color = 0U;
                            }

                            if (ImGui::TableNextColumn())  // category
                            {
                                ImGui::Selectable(diff_first_mark_ptr->category.c_str(), &selected, ImGuiSelectableFlags_SpanAllColumns);
                                CheckItem(diff_first_mark_ptr);
                            }
                            if (ImGui::TableNextColumn())  // name
                            {
                                ImGui::Selectable(diff_first_mark_ptr->name.c_str(), &selected, ImGuiSelectableFlags_SpanAllColumns);
                                CheckItem(diff_first_mark_ptr);
                            }
                            if (ImGui::TableNextColumn())  // first value
                            {
                                if (diff_first_mark_ptr->string.empty()) {
                                    ImGui::Text("%f", diff_first_mark_ptr->value);
                                } else {
                                    if (diff_first_mark_ptr->status == LogEngine::sc_START_ZONE) {
                                        ImGui::Text(ICON_FONT_ARROW_RIGHT_THICK " %s", diff_first_mark_ptr->string.c_str());
                                    } else if (diff_first_mark_ptr->status == LogEngine::sc_END_ZONE) {
                                        ImGui::Text("%s " ICON_FONT_ARROW_LEFT_THICK, diff_first_mark_ptr->string.c_str());
                                    } else {
                                        ImGui::Text("%s", diff_first_mark_ptr->string.c_str());
                                    }
                                }
                                CheckItem(diff_first_mark_ptr);
                            }
                            if (ImGui::TableNextColumn())  // second value
                            {
                                if (diff_second_mark_ptr->string.empty()) {
                                    ImGui::Text("%f", diff_second_mark_ptr->value);
                                } else {
                                    if (diff_second_mark_ptr->status == LogEngine::sc_START_ZONE) {
                                        ImGui::Text(ICON_FONT_ARROW_RIGHT_THICK " %s", diff_second_mark_ptr->string.c_str());
                                    } else if (diff_second_mark_ptr->status == LogEngine::sc_END_ZONE) {
                                        ImGui::Text(ICON_FONT_ARROW_LEFT_THICK " %s", diff_second_mark_ptr->string.c_str());
                                    } else {
                                        ImGui::Text("%s", diff_second_mark_ptr->string.c_str());
                                    }
                                }
                                CheckItem(diff_second_mark_ptr);
                            }

                            if (color) {
                                ImGui::PopStyleColor(count_color_push);
                            }
                        }
                    }
                }
                m_VirtualClipper.End();

                ImGui::EndTable();
            }
        }
    }
}

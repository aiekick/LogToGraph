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

#include "SignalsHoveredList.h"
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

void SignalsHoveredList::Clear() {}

bool SignalsHoveredList::Init() {
    return true;
}

void SignalsHoveredList::Unit() {}

bool SignalsHoveredList::DrawPanes(const uint32_t& /*vCurrentFrame*/, bool* vOpened, ImGuiContext* vContextPtr, void* /*vUserDatas*/) {
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
                if (ImGui::BeginMenuBar()) {
                    DrawMenuBar();
                    ImGui::EndMenuBar();
                }
                DrawTable();
            }
        }

        ImGui::End();
    }
    return change;
}

int SignalsHoveredList::CalcSignalsButtonCountAndSize(
    ImVec2& vOutCellSize, /* cell size						*/
    ImVec2& vOutButtonSize) /* button size (cell - paddings)	*/
{
    float aw = ImGui::GetContentRegionAvail().x;

    float width = ProjectFile::Instance()->m_SignalPreview_SizeX;

    int count = (int)(aw / ez::maxi(width, 1.0f));
    width = aw / (float)ez::maxi(count, 1);

    ProjectFile::Instance()->m_SignalPreview_CountX = count;

    if (count > 0) {
        vOutCellSize = ImVec2(width, width);
        vOutButtonSize = vOutCellSize - ImGui::GetStyle().ItemSpacing - ImGui::GetStyle().FramePadding * 2.0f;
    }

    return count;
}

void SignalsHoveredList::DrawMenuBar() {
    if (ImGui::BeginMenu("Settings")) {
        if (ImGui::MenuItem("Show variable signals only", nullptr, &ProjectFile::Instance()->m_ShowVariableSignalsInHoveredListView)) {
            ProjectFile::Instance()->SetProjectChange();
            LogEngine::Instance()->SetHoveredTime(LogEngine::Instance()->GetHoveredTime(), true);
        }

        {  // color of updated text rect
            if (ImGui::ContrastedButton("R##ResetSelectionColor")) {
                ProjectFile::Instance()->SetProjectChange();
                ProjectFile::Instance()->m_GraphColors.graphHoveredUpdatedRectColor = s_DefaultGraphColors.graphHoveredUpdatedRectColor;
            }
            ImGui::SameLine();
            if (ImGui::ColorEdit4(
                    "Updated values rect color##ResetSelectionColor",  //
                    &ProjectFile::Instance()->m_GraphColors.graphHoveredUpdatedRectColor.x,
                    ImGuiColorEditFlags_NoInputs)) {
                ProjectFile::Instance()->SetProjectChange();
            }
        }

        if (ImGui::SliderFloatDefault(
                150.0f, "Updated values rect thickness", &ProjectFile::Instance()->m_HoveredListChangedTextRectThickNess, 0.01f, 10.0f, 2.0f)) {
        }

        ImGui::EndMenu();
    }
}

void SignalsHoveredList::DrawTable() {
    auto win = ImGui::GetCurrentWindowRead();
    if (win) {
        const auto& signals_count = LogEngine::Instance()->GetPreviewTicks().size();
        if (signals_count) {
            static ImGuiTableFlags flags = ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_RowBg | ImGuiTableFlags_Hideable | ImGuiTableFlags_ScrollY |
                ImGuiTableFlags_NoHostExtendY | ImGuiTableFlags_Resizable;

            auto listViewID = ImGui::GetID("##SignalsHoveredList_DrawTable");
            if (ImGui::BeginTableEx("##SignalsHoveredList_DrawTable", listViewID, 5, flags))  //-V112
            {
                ImGui::TableSetupScrollFreeze(0, 1);  // Make header always visible
                ImGui::TableSetupColumn("Epoch", ImGuiTableColumnFlags_WidthFixed);
                ImGui::TableSetupColumn("Date", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_DefaultHide);
                ImGui::TableSetupColumn("Cat", ImGuiTableColumnFlags_WidthFixed);
                ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthFixed);

                ImGui::TableHeadersRow();

                const auto& updatedRectColor = ProjectFile::Instance()->m_GraphColors.graphHoveredUpdatedRectColor;
                const auto& updatedRectOffset = ImVec2(2.0f, 2.0f);
                const auto updatedRectTickness = ProjectFile::Instance()->m_HoveredListChangedTextRectThickNess;

                int32_t count_color_push = 0;
                ImU32 color = 0U;
                bool selected = false;
                m_VirtualClipper.Begin((int)signals_count, ImGui::GetTextLineHeightWithSpacing());
                while (m_VirtualClipper.Step()) {
                    for (int i = m_VirtualClipper.DisplayStart; i < m_VirtualClipper.DisplayEnd; ++i) {
                        if (i < 0)
                            continue;

                        const auto infos_ptr = LogEngine::Instance()->GetPreviewTicks().at((size_t)i).lock();
                        if (infos_ptr) {
                            ImGui::TableNextRow();

                            selected = LogEngine::Instance()->isSignalShown(infos_ptr->category, infos_ptr->name, &color);
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

                            if (ImGui::TableNextColumn()) {  // time
                                ImGui::Selectable(ez::str::toStr("%f", infos_ptr->time_epoch).c_str(), &selected, ImGuiSelectableFlags_SpanAllColumns);
                            }
                            if (ImGui::TableNextColumn()) {  // date time
                                ImGui::Selectable(infos_ptr->time_date_time.c_str(), &selected, ImGuiSelectableFlags_SpanAllColumns);
                            }
                            if (ImGui::TableNextColumn()) {  // category
                                ImGui::Selectable(infos_ptr->category.c_str(), &selected, ImGuiSelectableFlags_SpanAllColumns);
                            }
                            if (ImGui::TableNextColumn()) {  // name
                                ImGui::Selectable(infos_ptr->name.c_str(), &selected, ImGuiSelectableFlags_SpanAllColumns);
                            }
                            if (ImGui::TableNextColumn()) {  // value
                                // clang-format off
                                if (infos_ptr->string.empty()) {
                                    ImGui::DrawRectOverText(infos_ptr->just_changed, 
                                        updatedRectColor, updatedRectOffset, updatedRectTickness, 
                                        "%f", infos_ptr->value);
                                } else {
                                    if (infos_ptr->status == LogEngine::sc_START_ZONE) {
                                        ImGui::DrawRectOverText(infos_ptr->just_changed, 
                                            updatedRectColor, updatedRectOffset, updatedRectTickness, 
                                            ICON_FONT_ARROW_RIGHT_THICK " %s", infos_ptr->string.c_str());
                                    } else if (infos_ptr->status == LogEngine::sc_END_ZONE) {
                                        ImGui::DrawRectOverText(infos_ptr->just_changed, 
                                            updatedRectColor, updatedRectOffset, updatedRectTickness, 
                                            "%s " ICON_FONT_ARROW_LEFT_THICK, infos_ptr->string.c_str());
                                    } else {
                                        ImGui::DrawRectOverText(infos_ptr->just_changed, 
                                            updatedRectColor, updatedRectOffset, updatedRectTickness, 
                                            "%s", infos_ptr->string.c_str());
                                    }
                                }
                                // clang-format on
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

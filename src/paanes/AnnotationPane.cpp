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

#include "AnnotationPane.h"
#include <EzLibs/EzLog.hpp>

#include <Project/ProjectFile.h>
#include <cinttypes>  // printf zu
#include <models/graphs/GraphAnnotationModel.h>
#include <models/graphs/GraphAnnotation.h>
#include <models/log/SignalSerie.h>
#include <models/log/LogEngine.h>
#include <panes/ToolPane.h>
#include <panes/GraphListPane.h>
#include <res/fontIcons.h>

///////////////////////////////////////////////////////////////////////////////////
//// OVERRIDES ////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////

bool AnnotationPane::Init() {
    return true;
}

void AnnotationPane::Unit() {}

bool AnnotationPane::DrawPanes(const uint32_t& /*vCurrentFrame*/, bool* vOpened, ImGuiContext* vContextPtr, void* /*vUserDatas*/) {
    ImGui::SetCurrentContext(vContextPtr);
    bool change = false;
    if (vOpened != nullptr && *vOpened) {
        static ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_MenuBar;
        if (ImGui::Begin(GetName().c_str(), vOpened, flags)) {
#ifdef USE_DECORATIONS_FOR_RESIZE_CHILD_WINDOWS
            auto win = ImGui::GetCurrentWindowRead();
            if (win->Viewport->Idx != 0)
                flags |= ImGuiWindowFlags_NoResize;  // | ImGuiWindowFlags_NoTitleBar;
            else
                flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_MenuBar;
#endif
            if (ProjectFile::Instance()->IsProjectLoaded()) {
                if (ImGui::BeginMenuBar()) {
                    ImGui::EndMenuBar();
                }

                DrawContent();
            }
        }

        ImGui::End();
    }
    return change;
}

///////////////////////////////////////////////////////////////////////////////////
//// PRIVATE //////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////

void AnnotationPane::DrawContent() {
    static ImGuiTableFlags flags = ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_RowBg |
        // ImGuiTableFlags_Hideable |
        ImGuiTableFlags_ScrollY |
        // ImGuiTableFlags_Resizable |
        ImGuiTableFlags_NoHostExtendY;

    GraphAnnotationPtr annotation_to_remove_ptr = nullptr;

    auto listViewID = ImGui::GetID("##LogPane_DrawTable");
    if (ImGui::BeginTableEx("##LogPane_DrawTable", listViewID, 3, flags))  //-V112
    {
        ImGui::TableSetupScrollFreeze(0, 1);  // Make header always visible
        ImGui::TableSetupColumn("Delete", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("Signal Name", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Annotation", ImGuiTableColumnFlags_WidthFixed);

        ImGui::TableNextRow(ImGuiTableRowFlags_Headers);

        for (int column = 0; column < 3; column++)  //-V112
        {
            ImGui::TableSetColumnIndex(column);
            const char* column_name = ImGui::TableGetColumnName(column);  // Retrieve name passed to TableSetupColumn()
            ImGui::PushID(column);
            ImGui::TableHeader(column_name);
            ImGui::PopID();
        }

        uint32_t count_color_push = 0U;
        ImU32 color = 0U;
        bool selected = false;
        const auto _count_annotations = GraphAnnotationModel::Instance()->size();
        m_AnnotationsListClipper.Begin((int)_count_annotations, ImGui::GetTextLineHeightWithSpacing());
        while (m_AnnotationsListClipper.Step()) {
            for (int i = m_AnnotationsListClipper.DisplayStart; i < m_AnnotationsListClipper.DisplayEnd; ++i) {
                if (i < 0)
                    continue;

                auto& anno_ptr = GraphAnnotationModel::Instance()->at((size_t)i);
                if (anno_ptr) {
                    ImGui::TableNextRow();

                    selected = false;
                    auto signal_ptr = anno_ptr->GetParentSignalSerie().lock();
                    if (signal_ptr) {
                        selected = signal_ptr->show;
                        color = signal_ptr->color_u32;

                        if (anno_ptr->GetImGuiLabel()) {
                            if (selected && color) {
                                ImGui::PushStyleColor(ImGuiCol_Header, (ImU32)color);
                                ImGui::PushStyleColor(ImGuiCol_HeaderActive, (ImU32)color);
                                ImGui::PushStyleColor(ImGuiCol_HeaderHovered, (ImU32)color);
                                count_color_push = 3U;
                                if (ImGui::PushStyleColorWithContrast1(
                                        ImGuiCol_Header, ImGuiCol_Text, ImGui::CustomStyle::puContrastedTextColor, ImGui::CustomStyle::puContrastRatio)) {
                                    count_color_push = 4U;
                                }
                            } else {
                                color = 0U;
                            }

                            if (ImGui::TableNextColumn())  // delete
                            {
                                if (ImGui::ContrastedButton(ICON_FONT_CANCEL)) {
                                    annotation_to_remove_ptr = anno_ptr;
                                }
                            }
                            if (ImGui::TableNextColumn())  // signal name
                            {
                                ImGui::Selectable(signal_ptr->name.c_str(), &selected, ImGuiSelectableFlags_SpanAllColumns);
                                CheckItem(signal_ptr);
                            }
                            if (ImGui::TableNextColumn())  // annotation label
                            {
                                ImGui::Selectable(anno_ptr->GetImGuiLabel(), &selected, ImGuiSelectableFlags_SpanAllColumns);
                                CheckItem(signal_ptr);
                            }

                            if (color) {
                                ImGui::PopStyleColor(count_color_push);
                            }
                        }
                    }
                }
            }
        }
        m_AnnotationsListClipper.End();

        ImGui::EndTable();
    }

    if (annotation_to_remove_ptr) {
        GraphAnnotationModel::Instance()->erase(annotation_to_remove_ptr);
        annotation_to_remove_ptr = nullptr;
    }
}

void AnnotationPane::CheckItem(SignalSeriePtr vSignalSeriePtr) {
    if (ImGui::IsItemHovered() && vSignalSeriePtr) {
        if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
            // tofix : to centralize in a mvc controller
            LogEngine::Instance()->ShowHideSignal(vSignalSeriePtr->category, vSignalSeriePtr->name);
            ProjectFile::Instance()->SetProjectChange();
            ToolPane::Instance()->UpdateTree();
            GraphListPane::Instance()->UpdateDB();
        }
    }
}

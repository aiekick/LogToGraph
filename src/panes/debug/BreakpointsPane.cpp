// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include "BreakpointsPane.h"
#include <imgui_internal.h>
#include <project/ProjectFile.h>
#include <models/debug/ScriptDebugger.h>

#include <vector>
#include <cstdint>
#include <algorithm>

bool BreakpointsPane::init() {
    return true;
}

void BreakpointsPane::unit() {}

///////////////////////////////////////////////////////////////////////////////////
//// IMGUI PANE ///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////

bool BreakpointsPane::drawPanes(bool* apOpened, LayoutPaneUserDatas apUserDatas) {
    bool change = false;
    if (apOpened != nullptr && *apOpened) {
        static ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_MenuBar;
        if (ImGui::Begin(getName().c_str(), apOpened, flags)) {
#ifdef USE_DECORATIONS_FOR_RESIZE_CHILD_WINDOWS
            auto win = ImGui::GetCurrentWindowRead();
            if (win->Viewport->Idx != 0)
                flags |= ImGuiWindowFlags_NoResize;  // | ImGuiWindowFlags_NoTitleBar;
            else
                flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_MenuBar;
#endif
            if (ProjectFile::ref()->IsProjectLoaded()) {
            if (ImGui::BeginMenuBar()) {
                ImGui::EndMenuBar();
            }
            const auto& scriptFile = ScriptDebugger::ref()->getScriptFilePathName();
            const auto& breakpoints = ScriptDebugger::ref()->getBreakpoints();

            if (!scriptFile.empty()) {
                ImGui::TextWrapped("%s", scriptFile.c_str());
            }
            if (ImGui::ContrastedSmallButton("Clear all")) {
                ScriptDebugger::ref()->clearBreakpoints();
            }
            ImGui::Separator();

            std::vector<int32_t> sortedLines(breakpoints.begin(), breakpoints.end());
            std::sort(sortedLines.begin(), sortedLines.end());

            static ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY;
            if (ImGui::BeginTable("##breakpoints", 2, flags)) {
                ImGui::TableSetupColumn("Line");
                ImGui::TableSetupColumn("");
                ImGui::TableHeadersRow();
                for (const auto& line : sortedLines) {
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("%d", line);
                    ImGui::TableNextColumn();
                    ImGui::PushID(line);
                    if (ImGui::ContrastedSmallButton("remove")) {
                        ScriptDebugger::ref()->setBreakpoint(scriptFile, line, false);
                    }
                    ImGui::PopID();
                }
                ImGui::EndTable();
            }
            }  // IsProjectLoaded
        }
        ImGui::End();
    }
    return change;
}

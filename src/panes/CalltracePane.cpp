// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include "CalltracePane.h"
#include <imgui_internal.h>
#include <models/debug/ScriptDebugger.h>

bool CalltracePane::init() {
    return true;
}

void CalltracePane::unit() {}

///////////////////////////////////////////////////////////////////////////////////
//// IMGUI PANE ///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////

bool CalltracePane::drawPanes(bool* apOpened, LayoutPaneUserDatas apUserDatas) {
    bool change = false;
    if (apOpened != nullptr && *apOpened) {
        if (ImGui::Begin(getName().c_str(), apOpened, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus)) {
            const bool isPaused = (ScriptDebugger::ref()->getMode() == ScriptDebugger::Mode::Paused);
            if (!isPaused) {
                ImGui::TextDisabled("Not paused");
            } else {
                const auto state = ScriptDebugger::ref()->getState();
                static ImGuiTableFlags flags =
                    ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY;
                if (ImGui::BeginTable("##calltrace", 3, flags)) {
                    ImGui::TableSetupColumn("Function");
                    ImGui::TableSetupColumn("Source");
                    ImGui::TableSetupColumn("Line");
                    ImGui::TableHeadersRow();
                    for (const auto& frame : state.callStack) {
                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();
                        ImGui::TextUnformatted(frame.function.empty() ? "?" : frame.function.c_str());
                        ImGui::TableNextColumn();
                        ImGui::TextUnformatted(frame.source.c_str());
                        ImGui::TableNextColumn();
                        ImGui::Text("%d", frame.line);
                    }
                    ImGui::EndTable();
                }
            }
        }
        ImGui::End();
    }
    return change;
}

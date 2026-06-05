// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include "WatcherPane.h"
#include <imgui_internal.h>
#include <models/debug/ScriptDebugger.h>

bool WatcherPane::init() {
    return true;
}

void WatcherPane::unit() {}

///////////////////////////////////////////////////////////////////////////////////
//// IMGUI PANE ///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////

bool WatcherPane::drawPanes(bool* apOpened, LayoutPaneUserDatas apUserDatas) {
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
                if (ImGui::BeginTable("##watcher", 3, flags)) {
                    ImGui::TableSetupColumn("Name");
                    ImGui::TableSetupColumn("Value");
                    ImGui::TableSetupColumn("Type");
                    ImGui::TableHeadersRow();
                    for (const auto& localVar : state.locals) {
                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();
                        ImGui::TextUnformatted(localVar.name.c_str());
                        ImGui::TableNextColumn();
                        ImGui::TextUnformatted(localVar.value.c_str());
                        ImGui::TableNextColumn();
                        ImGui::TextUnformatted(localVar.typeName.c_str());
                    }
                    for (const auto& upvalueVar : state.upvalues) {
                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();
                        ImGui::TextUnformatted((upvalueVar.name + " (upvalue)").c_str());
                        ImGui::TableNextColumn();
                        ImGui::TextUnformatted(upvalueVar.value.c_str());
                        ImGui::TableNextColumn();
                        ImGui::TextUnformatted(upvalueVar.typeName.c_str());
                    }
                    ImGui::EndTable();
                }
            }
        }
        ImGui::End();
    }
    return change;
}

// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include "StackTreePane.h"
#include <imgui_internal.h>
#include <models/debug/ScriptDebugger.h>
#include <panes/DebugVarTree.h>

#include <string>

bool StackTreePane::init() {
    return true;
}

void StackTreePane::unit() {}

///////////////////////////////////////////////////////////////////////////////////
//// IMGUI PANE ///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////

bool StackTreePane::drawPanes(bool* apOpened, LayoutPaneUserDatas apUserDatas) {
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
            if (ImGui::BeginMenuBar()) {
                ImGui::EndMenuBar();
            }
            const bool isPaused = (ScriptDebugger::ref()->getMode() == ScriptDebugger::Mode::Paused);
            if (!isPaused) {
                ImGui::TextDisabled("Not paused");
            } else {
                const auto state = ScriptDebugger::ref()->getState();
                static ImGuiTableFlags flags =
                    ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY;
                if (ImGui::BeginTable("##stacktree", 5, flags)) {
                    LtgDebugUI::setupTreeColumns();

                    int32_t uid = 0;

                    // Stack root -> frames -> Locals / Upvalues -> vars
                    if (!state.callStack.empty()) {
                        const std::string stackItems = std::to_string(state.callStack.size()) + " Items";
                        const bool stackOpen = LtgDebugUI::beginTreeRow("Stack", 1, 1, nullptr, nullptr, stackItems.c_str(), uid, true, false);
                        if (stackOpen) {
                            int32_t frameIndex = 1;
                            for (const auto& frame : state.callStack) {
                                const std::string frameLabel = "[" + std::to_string(frameIndex - 1) + "] " + (frame.function.empty() ? "?" : frame.function);
                                const std::string frameValue = frame.source + ":" + std::to_string(frame.line);
                                const bool frameLeaf = frame.locals.empty() && frame.upvalues.empty();
                                const bool frameOpen =
                                    LtgDebugUI::beginTreeRow(frameLabel.c_str(), 2, frameIndex, nullptr, nullptr, frameValue.c_str(), uid, frameIndex == 1, frameLeaf);
                                if (frameOpen && !frameLeaf) {
                                    if (!frame.locals.empty()) {
                                        LtgDebugUI::drawVarGroup("Locals", 3, 1, frame.locals, uid);
                                    }
                                    if (!frame.upvalues.empty()) {
                                        LtgDebugUI::drawVarGroup("Upvalues", 3, 2, frame.upvalues, uid);
                                    }
                                    ImGui::TreePop();
                                }
                                ImGui::PopID();
                                ++frameIndex;
                            }
                            ImGui::TreePop();
                        }
                        ImGui::PopID();
                    }

                    // Globals root
                    if (!state.globals.empty()) {
                        LtgDebugUI::drawVarGroup("Globals", 1, 2, state.globals, uid);
                    }

                    ImGui::EndTable();
                }
            }
        }
        ImGui::End();
    }
    return change;
}

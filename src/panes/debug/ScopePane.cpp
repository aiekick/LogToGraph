// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include "ScopePane.h"
#include <imgui_internal.h>
#include <project/ProjectFile.h>
#include <models/debug/ScriptDebugger.h>
#include <panes/debug/DebugVarTree.h>

#include <vector>

bool ScopePane::init() {
    return true;
}

void ScopePane::unit() {}

void ScopePane::Clear() {
    m_LastStateRevision = -1;
    m_StateCache = Ltg::DebugState{};
}

///////////////////////////////////////////////////////////////////////////////////
//// IMGUI PANE ///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////

bool ScopePane::drawPanes(bool* apOpened, LayoutPaneUserDatas apUserDatas) {
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
            const bool isPaused = (ScriptDebugger::ref()->getMode() == ScriptDebugger::Mode::Paused);
            if (!isPaused) {
                ImGui::TextDisabled("Not paused");
            } else {
                const int64_t stateRevision = ScriptDebugger::ref()->getStateRevision();
                if (stateRevision != m_LastStateRevision) {
                    m_StateCache = ScriptDebugger::ref()->getState();
                    m_LastStateRevision = stateRevision;
                }
                const auto& state = m_StateCache;
                if (state.callStack.empty()) {
                    ImGui::TextDisabled("No frame");
                } else {
                    // the current scope is the innermost frame (where execution is paused).
                    // keep only NAMED variables: the (*temporary) slots and internal (for ...)
                    // loop vars are left to the Stack Tree pane.
                    const auto& frame = state.callStack.front();
                    std::vector<Ltg::DebugVar> namedLocals;
                    for (const auto& var : frame.locals) {
                        if (!var.name.empty() && var.name[0] != '(') {
                            namedLocals.push_back(var);
                        }
                    }
                    std::vector<Ltg::DebugVar> namedUpvalues;
                    for (const auto& var : frame.upvalues) {
                        if (!var.name.empty() && var.name[0] != '(') {
                            namedUpvalues.push_back(var);
                        }
                    }
                    static ImGuiTableFlags flags =
                        ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY;
                    if (ImGui::BeginTable("##scope", 5, flags)) {
                        LtgDebugUI::setupTreeColumns();
                        int32_t uid = 0;
                        if (!namedLocals.empty()) {
                            LtgDebugUI::drawVarGroup("Locals", 1, 1, namedLocals, uid);
                        }
                        if (!namedUpvalues.empty()) {
                            LtgDebugUI::drawVarGroup("Upvalues", 1, 2, namedUpvalues, uid);
                        }
                        ImGui::EndTable();
                    }
                }
            }
            }  // IsProjectLoaded
        }
        ImGui::End();
    }
    return change;
}

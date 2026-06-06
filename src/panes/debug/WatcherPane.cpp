// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include "WatcherPane.h"
#include <imgui_internal.h>
#include <project/ProjectFile.h>
#include <models/debug/ScriptDebugger.h>

#include <cstring>

bool WatcherPane::init() {
    m_Entries.reserve(64);
    return true;
}

void WatcherPane::unit() {}

void WatcherPane::Clear() {
    m_Entries.clear();
    m_NextEvalId = 0;
    m_NewExpressionBuf[0] = '\0';
}

void WatcherPane::AddExpression(const std::string& aExpression) {
    if (aExpression.empty()) {
        return;
    }
    WatchEntry entry;
    entry.expression = aExpression;
    entry.evalId = m_NextEvalId++;
    m_Entries.push_back(entry);
}

///////////////////////////////////////////////////////////////////////////////////
//// IMGUI PANE ///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////

bool WatcherPane::drawPanes(bool* apOpened, LayoutPaneUserDatas apUserDatas) {
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

                // manual-add row: input + Add button. only meaningful while paused, but the user can
                // still type and queue an expression that will resolve at the next pause.
                ImGui::TextUnformatted("Add expression:");
                ImGui::SameLine();
                ImGui::SetNextItemWidth(-100.0f);
                const bool submitted = ImGui::InputText("##watch_input", m_NewExpressionBuf, IM_ARRAYSIZE(m_NewExpressionBuf),
                                                        ImGuiInputTextFlags_EnterReturnsTrue);
                ImGui::SameLine();
                if (ImGui::ContrastedButton("Add", nullptr, nullptr, -1.0f) || submitted) {
                    AddExpression(m_NewExpressionBuf);
                    m_NewExpressionBuf[0] = '\0';
                }

                // re-issue an eval request for each entry that hasn't resolved YET this pause. results
                // are invalidated by ScriptDebugger::onPause on the next break, so the loop kicks in
                // again automatically after a resume → re-pause cycle.
                if (isPaused) {
                    for (auto& entry : m_Entries) {
                        if (!entry.hasResult) {
                            Ltg::EvalResult result;
                            if (ScriptDebugger::ref()->getEvalResult(entry.evalId, result)) {
                                entry.lastResult = result;
                                entry.hasResult = true;
                            } else {
                                ScriptDebugger::ref()->requestEval(entry.evalId, entry.expression);
                            }
                        }
                    }
                } else {
                    // not paused → invalidate every entry's cached result so we re-eval at next pause
                    for (auto& entry : m_Entries) {
                        entry.hasResult = false;
                    }
                }

                ImGui::Separator();

                static ImGuiTableFlags tableFlags =
                    ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY;
                if (ImGui::BeginTable("##watches", 4, tableFlags)) {
                    ImGui::TableSetupColumn("Expression");
                    ImGui::TableSetupColumn("Value");
                    ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed);
                    ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed);
                    ImGui::TableHeadersRow();

                    size_t toRemove = static_cast<size_t>(-1);
                    for (size_t i = 0; i < m_Entries.size(); ++i) {
                        const auto& entry = m_Entries[i];
                        ImGui::TableNextRow();
                        ImGui::PushID(static_cast<int>(i));

                        ImGui::TableNextColumn();
                        ImGui::TextUnformatted(entry.expression.c_str());

                        ImGui::TableNextColumn();
                        if (!isPaused) {
                            ImGui::TextDisabled("(not paused)");
                        } else if (!entry.hasResult) {
                            ImGui::TextDisabled("evaluating...");
                        } else if (!entry.lastResult.error.empty()) {
                            ImGui::TextColored(ImVec4(0.9f, 0.3f, 0.3f, 1.0f), "%s", entry.lastResult.error.c_str());
                        } else {
                            ImGui::TextUnformatted(entry.lastResult.value.c_str());
                        }

                        ImGui::TableNextColumn();
                        if (isPaused && entry.hasResult && entry.lastResult.error.empty()) {
                            ImGui::TextUnformatted(entry.lastResult.typeName.c_str());
                        }

                        ImGui::TableNextColumn();
                        if (ImGui::ContrastedSmallButton("remove")) {
                            toRemove = i;
                        }

                        ImGui::PopID();
                    }
                    if (toRemove != static_cast<size_t>(-1)) {
                        m_Entries.erase(m_Entries.begin() + static_cast<ptrdiff_t>(toRemove));
                    }
                    ImGui::EndTable();
                }
            }
        }
        ImGui::End();
    }
    return change;
}

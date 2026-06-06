// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include <panes/misc/CodePane.h>
#include <cinttypes>  // printf zu

#include <ezlibs/ezLog.hpp>
#include <ezlibs/ezFile.hpp>

#include <res/fontIcons.h>
#include <models/debug/ScriptDebugger.h>
#include <models/script/ScriptingEngine.h>
#include <project/ProjectFile.h>

bool CodePane::init() {
    // for avoid a reallocation of the vector for each push/emplace
    // where the the language Type in each editor got corrupted
    // because passed by ref
    // 1000 editor is sufficient for our need
    m_CodeSheets.reserve(1000U);
    return true;
}

void CodePane::unit() {
    m_CodeSheets.clear();
}

void CodePane::Clear() {
    m_CodeSheets.clear();
    m_BreakpointsRevisionSeen = -1;
    m_DebugScriptFileCache.clear();
    m_Breakpoints0BasedCache.clear();
    m_LastStateRevision = -1;
    m_StateCache = Ltg::DebugState{};
}

///////////////////////////////////////////////////////////////////////////////////
//// IMGUI PANE ///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////

bool CodePane::drawPanes(bool* apOpened, LayoutPaneUserDatas apUserDatas) {
    
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
            m_DrawDebugToolbar();

            // refresh the breakpoint render cache ONLY when the debugger's revision has advanced.
            // hot path = atomic load + int compare; cold path (rare) = mutex + set copy + 0-based transform.
            const int64_t breakpointsRevision = ScriptDebugger::ref()->getBreakpointsRevision();
            if (breakpointsRevision != m_BreakpointsRevisionSeen) {
                m_DebugScriptFileCache = ScriptDebugger::ref()->getScriptFilePathName();
                const auto& breakpoints1Based = ScriptDebugger::ref()->getBreakpoints();
                m_Breakpoints0BasedCache.clear();
                for (const auto& breakpointLine : breakpoints1Based) {
                    m_Breakpoints0BasedCache.insert(breakpointLine - 1);
                }
                m_BreakpointsRevisionSeen = breakpointsRevision;
            }

            // same pattern for the paused snapshot — getStateRevision() bumps once per pause event
            const int64_t stateRevision = ScriptDebugger::ref()->getStateRevision();
            if (stateRevision != m_LastStateRevision) {
                m_StateCache = ScriptDebugger::ref()->getState();
                m_LastStateRevision = stateRevision;
            }

            const bool isPaused = (ScriptDebugger::ref()->getMode() == ScriptDebugger::Mode::Paused);
            const bool isDebugArmed = ScriptDebugger::ref()->isDebugArmed();
            int32_t currentExecLine0Based = -1;
            if (isPaused) {
                currentExecLine0Based = m_StateCache.line - 1;
            }

            if (ImGui::BeginTabBar("CodePane")) {
                for (auto& sheet : m_CodeSheets) {
                    // editing the project script marks the project dirty (so the Save button shows)
                    if (sheet.filepathName == sc_PROJECT_SCRIPT_ID && sheet.codeEditor.IsModified() &&
                        !ProjectFile::ref()->IsThereAnyProjectChanges()) {
                        ProjectFile::ref()->SetProjectChange(true);
                    }
                    ImGui::PushID(sheet.filepathName.c_str());
                    if (ImGui::BeginTabItem(sheet.title.c_str(), &sheet.opened)) {
                        if (sheet.filepathName == m_DebugScriptFileCache) {
                            sheet.codeEditor.SetBreakpoints(m_Breakpoints0BasedCache, breakpointsRevision);
                            sheet.codeEditor.SetCurrentExecLine(currentExecLine0Based);
                        }
                        sheet.codeEditor.SetBreakpointInteractionEnabled(isDebugArmed);
                        sheet.codeEditor.OnImGui();
                        ImGui::EndTabItem();
                    }
                    ImGui::PopID();
                }
                ImGui::EndTabBar();
            }
            }  // IsProjectLoaded
        }

        ImGui::End();
    }
    return change;
}

void CodePane::m_DrawDebugToolbar() {
    const bool isPaused = (ScriptDebugger::ref()->getMode() == ScriptDebugger::Mode::Paused);
    const bool workerBusy = ScriptingEngine::ref()->IsJoinable();

    bool armed = ScriptDebugger::ref()->isDebugArmed();
    if (ImGui::ToggleContrastedButton(ICON_FONT_BUG " Debug (on)", ICON_FONT_BUG " Debug (off)", &armed, "Arm the script debugger")) {
        ScriptDebugger::ref()->setDebugArmed(armed);
    }

    // Run / Continue
    ImGui::SameLine();
    ImGui::BeginDisabled(!armed || (workerBusy && !isPaused));
    if (ImGui::ContrastedButton(ICON_FONT_PLAY "##dbg_run", isPaused ? "Continue" : "Run (analyse the log files)")) {
        if (isPaused) {
            ScriptDebugger::ref()->doContinue();
        } else if (!workerBusy) {
            m_StartAnalyse();
        }
    }
    ImGui::EndDisabled();

    ImGui::SameLine();
    ImGui::BeginDisabled(!armed || !isPaused);
    if (ImGui::ContrastedButton(ICON_FONT_DEBUG_STEP_INTO "##dbg_stepinto", "Step into")) {
        ScriptDebugger::ref()->stepInto();
    }
    ImGui::SameLine();
    if (ImGui::ContrastedButton(ICON_FONT_DEBUG_STEP_OVER "##dbg_stepover", "Step over")) {
        ScriptDebugger::ref()->stepOver();
    }
    ImGui::SameLine();
    if (ImGui::ContrastedButton(ICON_FONT_DEBUG_STEP_OUT "##dbg_stepout", "Step out")) {
        ScriptDebugger::ref()->stepOut();
    }
    ImGui::EndDisabled();

    ImGui::SameLine();
    ImGui::BeginDisabled(!armed || !workerBusy || isPaused);
    if (ImGui::ContrastedButton(ICON_FONT_PAUSE "##dbg_pause", "Pause")) {
        ScriptDebugger::ref()->pause();
    }
    ImGui::EndDisabled();

    ImGui::SameLine();
    ImGui::BeginDisabled(!armed || !workerBusy);
    if (ImGui::ContrastedButton(ICON_FONT_STOP "##dbg_stop", "Stop")) {
        ScriptingEngine::s_working = false;  // break the parse loop on the worker thread
        ScriptDebugger::ref()->stop();
    }
    ImGui::EndDisabled();

    ImGui::SameLine();
    if (isPaused) {
        // m_StateCache filled at the top of drawPanes (same frame), no need to re-query getState()
        ImGui::Text("Paused | log row %d | line %d", m_StateCache.logRowIndex, m_StateCache.line);
    } else if (workerBusy) {
        ImGui::TextUnformatted("Running");
    } else {
        ImGui::TextUnformatted("Idle");
    }

    ImGui::Separator();
}

void CodePane::m_StartAnalyse() {
    // same as the ToolPane "Start Analyse": run the in-app script over the log files
    ScriptingEngine::ref()->Clear();
    ScriptingEngine::ref()->SetScriptCode(GetScriptCode());
    const auto& sources = ProjectFile::ref()->GetSourceFilePathNames();
    for (const auto& source : sources) {
        ScriptingEngine::ref()->AddSourceFilePathName(source.second);
    }
    ScriptingEngine::ref()->StartWorkerThread(false);
}

void CodePane::OpenFile(const std::string& vFilePathName, size_t vErrorLine, std::string vErrorMsg) {
    CodeSheet* existing_code_sheet_ptr = nullptr;
    for (auto& sheet : m_CodeSheets) {
        if (sheet.filepathName == vFilePathName) {
            existing_code_sheet_ptr = &sheet;
            break;
        }
    }

    auto ps = ez::file::parsePathFileName(vFilePathName);
    if (ps.isOk) {
        const auto code = ez::file::loadFileToString(vFilePathName);
        CodeEditorLanguage type = TextEditor::Language::C();
        if (ps.ext == "cpp" || ps.ext == "hpp") {
            type = TextEditor::Language::Cpp();
        } else if (ps.ext == "c" || ps.ext == "h") {
            type = TextEditor::Language::C();
        } else if (ps.ext == "lua") {
            type = TextEditor::Language::Lua();
        }
        if (existing_code_sheet_ptr != nullptr) {
            existing_code_sheet_ptr->wasModified = false;
            existing_code_sheet_ptr->opened = true;
            if (!existing_code_sheet_ptr->opened) {
                existing_code_sheet_ptr->wasModified = false;
                existing_code_sheet_ptr->codeEditor.SetCode(code, type);
            }
            existing_code_sheet_ptr->codeEditor.AddErrorMarker(vErrorLine, vErrorMsg);
        } else {
            auto& sheet = m_CodeSheets.emplace_back();
            sheet.codeEditor.init();
            sheet.filepathName = vFilePathName;
            sheet.opened = true;
            sheet.wasModified = false;
            sheet.title = ps.name + "." + ps.ext;
            // report gutter breakpoint toggles to the shared debugger (widget 0-based -> 1-based)
            const std::string filePathForCallback = vFilePathName;
            sheet.codeEditor.SetBreakpointToggledCallback([filePathForCallback](int32_t aLine, bool aAdd) {
                ScriptDebugger::ref()->setBreakpoint(filePathForCallback, aLine + 1, aAdd);
            });
            sheet.codeEditor.SetCode(code, type);
            sheet.codeEditor.AddErrorMarker(vErrorLine, vErrorMsg);
        }
    }
}

void CodePane::OpenScript(const std::string& aCode) {
    CodeSheet* scriptSheetPtr = nullptr;
    for (auto& sheet : m_CodeSheets) {
        if (sheet.filepathName == sc_PROJECT_SCRIPT_ID) {
            scriptSheetPtr = &sheet;
            break;
        }
    }
    if (scriptSheetPtr == nullptr) {
        auto& sheet = m_CodeSheets.emplace_back();
        sheet.codeEditor.init();
        sheet.filepathName = sc_PROJECT_SCRIPT_ID;
        sheet.title = ICON_FONT_CODE_BRACES " Script";
        sheet.opened = true;
        sheet.wasModified = false;
        // report gutter breakpoint toggles to the shared debugger (widget 0-based -> 1-based)
        const std::string scriptId = sc_PROJECT_SCRIPT_ID;
        sheet.codeEditor.SetBreakpointToggledCallback([scriptId](int32_t aLine, bool aAdd) {
            ScriptDebugger::ref()->setBreakpoint(scriptId, aLine + 1, aAdd);
        });
        // editor Ctrl+S (or its File > Save) persists the project script into the .ltg db
        sheet.codeEditor.SetSaveCallback([]() { ProjectFile::ref()->Save(); });
        scriptSheetPtr = &sheet;
    }
    scriptSheetPtr->codeEditor.SetCode(aCode, TextEditor::Language::Lua());
}

std::string CodePane::GetScriptCode() {
    for (auto& sheet : m_CodeSheets) {
        if (sheet.filepathName == sc_PROJECT_SCRIPT_ID) {
            return sheet.codeEditor.GetCode();
        }
    }
    return {};
}

void CodePane::MarkScriptSaved() {
    for (auto& sheet : m_CodeSheets) {
        if (sheet.filepathName == sc_PROJECT_SCRIPT_ID) {
            sheet.codeEditor.MarkSaved();
            return;
        }
    }
}

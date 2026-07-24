// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include <panes/misc/CodePane.h>
#include <cinttypes>  // printf zu

#include <ezlibs/ezLog.hpp>
#include <ezlibs/ezFile.hpp>

#include <fonts/fontIcons.h>
#include <models/debug/ScriptDebugger.h>
#include <models/script/ScriptingEngine.h>
#include <project/ProjectFile.h>
#include <panes/debug/BreakpointsPane.h>
#include <panes/debug/CalltracePane.h>
#include <panes/debug/StackTreePane.h>
#include <panes/debug/ScopePane.h>
#include <panes/debug/WatcherPane.h>
#include <settings/AppSettings.h>
#include <settings/DebugSettings.h>

#include <cmath>

bool CodePane::init() {
    // for avoid a reallocation of the vector for each push/emplace
    // where the the language Type in each editor got corrupted
    // m_CodeSheets is a std::list: node addresses are stable by construction, so the old
    // reserve()-to-avoid-relocation trick is no longer needed
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
    m_LastErrorsRevisionSeen = -1;
    m_ErrorsCache.clear();
    m_LastCompletionPushUndoIndex = -1;
    // wipe the plugin's completion state too — the previous project's user globals must not leak
    // into the next project's autocomplete. empty code hits the plugin's reset path which clears
    // the tracked user globals and leaves only the stdlib + ltg bindings.
    ScriptingEngine::ref()->SetProjectScriptCode(std::string());
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
            // menu bar — Debug submenu mirrors DebugSettings (same fields as the SettingsDialog "Debug"
            // section). always visible (even when no project is loaded) so the user can flip toggles
            // before opening a project. compact MenuItem checkboxes share the bool* with the dialog.
            if (ImGui::BeginMenuBar()) {
                if (ImGui::BeginMenu("Debug")) {
                    DebugSettings::ref()->drawMenuItems();
                    ImGui::EndMenu();
                }
                ImGui::EndMenuBar();
            }
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
                    // VS-style caret sync — every NEW pause event snaps the editor's text caret to the
                    // active line so Step* / Continue / breakpoint hits visually advance the caret in
                    // step with the gutter arrow. Between pauses the caret is free (user can click
                    // anywhere to inspect), but the next pause brings it back to the live execution.
                    if (m_StateCache.line > 0 && !m_StateCache.sourceFile.empty()) {
                        for (auto& sheet : m_CodeSheets) {
                            if (sheet.filepathName == m_StateCache.sourceFile) {
                                sheet.codeEditor.MoveCursorTo(m_StateCache.line - 1, 0);
                                break;
                            }
                        }
                    }
                }

                // scripting errors — refreshed when ScriptingEngine bumps GetErrorsRevision(). on a bump,
                // wipe all sheets' error markers then push the new ones to each matching sheet (route by err.file).
                // user-gated by DebugSettings — when off, we still wipe (so a previously-shown marker disappears
                // promptly) but skip the add step, so the editor stays clean across script runs.
                const int64_t errorsRevision = ScriptingEngine::ref()->GetErrorsRevision();
                const bool errorMarkersEnabled = DebugSettings::ref()->isErrorMarkersEnabled();
                if (errorsRevision != m_LastErrorsRevisionSeen) {
                    m_ErrorsCache = ScriptingEngine::ref()->GetLastRunErrors();
                    m_LastErrorsRevisionSeen = errorsRevision;
                    for (auto& sheet : m_CodeSheets) {
                        sheet.codeEditor.ClearErrorMarkers();
                    }
                    if (errorMarkersEnabled) {
                        for (const auto& errorEntry : m_ErrorsCache) {
                            for (auto& sheet : m_CodeSheets) {
                                if (sheet.filepathName == errorEntry.file) {
                                    sheet.codeEditor.AddErrorMarker(errorEntry.line, errorEntry.message);
                                }
                            }
                        }
                    }
                }
                // react in-place when the user toggles error markers without waiting for the next
                // ScriptingEngine errors-revision bump: off -> wipe; on -> re-apply from cache.
                if (errorMarkersEnabled != m_ErrorMarkersEnabledLastSeen) {
                    if (!errorMarkersEnabled) {
                        for (auto& sheet : m_CodeSheets) {
                            sheet.codeEditor.ClearErrorMarkers();
                        }
                    } else {
                        for (const auto& errorEntry : m_ErrorsCache) {
                            for (auto& sheet : m_CodeSheets) {
                                if (sheet.filepathName == errorEntry.file) {
                                    sheet.codeEditor.AddErrorMarker(errorEntry.line, errorEntry.message);
                                }
                            }
                        }
                    }
                    m_ErrorMarkersEnabledLastSeen = errorMarkersEnabled;
                }

                const bool isPaused = (ScriptDebugger::ref()->getMode() == ScriptDebugger::Mode::Paused);
                const bool isDebugArmed = ScriptDebugger::ref()->isDebugArmed();
                int32_t currentExecLine0Based = -1;
                if (isPaused) {
                    currentExecLine0Based = m_StateCache.line - 1;
                }

                // reset hover state before rendering — the editor's hover callback (set in OpenScript)
                // writes here when the mouse is over text. anything we read after EndTabBar is the
                // current frame's value (empty if mouse is off-editor or on whitespace/punctuation).
                m_HoveredToken.clear();

                // mouse stillness timer for VS-style hover delay: any non-zero MouseDelta this frame
                // resets the clock. the tooltip block reads `now - m_MouseStillSince` and compares to
                // AppSettings::getHoverDelaySec().
                const ImVec2 mouseDelta = ImGui::GetIO().MouseDelta;
                if (std::fabs(mouseDelta.x) > 0.0f || std::fabs(mouseDelta.y) > 0.0f) {
                    m_MouseStillSince = ImGui::GetTime();
                }

                if (ImGui::BeginTabBar("CodePane")) {
                    for (auto& sheet : m_CodeSheets) {
                        // editing the project script marks the project dirty (so the Save button shows)
                        if (sheet.filepathName == sc_PROJECT_SCRIPT_ID && sheet.codeEditor.IsModified() && !ProjectFile::ref()->IsThereAnyProjectChanges()) {
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
                            // capture interactive Ctrl+MouseWheel zoom on the project-script sheet
                            // and persist it via ProjectFile. Only the project-script sheet — external
                            // file sheets are transient and not persisted in the project XML.
                            if (sheet.filepathName == sc_PROJECT_SCRIPT_ID) {
                                const float liveScale = sheet.codeEditor.GetCurrentFontScale();
                                if (liveScale > 0.0f && std::fabs(liveScale - ProjectFile::ref()->m_ProjectScriptFontScale) > 0.001f) {
                                    ProjectFile::ref()->m_ProjectScriptFontScale = liveScale;
                                    ProjectFile::ref()->SetProjectChange(true);
                                }
                                // refresh the plugin's completion state when the editor's undo index advances
                                // (i.e. an actual edit happened). this surfaces top-level user globals (function
                                // parse(...), helpers = {...}, ...) in autocomplete next to the stdlib + ltg
                                // bindings. the push is cheap (single sandbox exec on the plugin's separate
                                // sol::state); the change is gated so we don't re-run it on idle frames.
                                // also user-gated by DebugSettings — when off (intended for very large scripts
                                // where re-exec is too heavy), the autocomplete falls back to the stdlib + ltg
                                // bindings only. re-enabling resumes pushes on the next edit (undo index has
                                // advanced past m_LastCompletionPushUndoIndex during the off period).
                                if (DebugSettings::ref()->isProjectScriptRecompileEnabled()) {
                                    const int64_t liveUndoIndex = static_cast<int64_t>(sheet.codeEditor.GetUndoIndex());
                                    if (liveUndoIndex != m_LastCompletionPushUndoIndex) {
                                        ScriptingEngine::ref()->SetProjectScriptCode(sheet.codeEditor.GetCode());
                                        m_LastCompletionPushUndoIndex = liveUndoIndex;
                                    }
                                }
                            }
                            ImGui::EndTabItem();
                        }
                        ImGui::PopID();
                    }
                    ImGui::EndTabBar();
                }

                // hover-eval tooltip — gated by the VS-style hover delay; only meaningful when paused
                // (otherwise no scope to evaluate in). delay value is live-tunable in Settings > General.
                // also user-gated by DebugSettings (off → no eval requested, no tooltip shown).
                if (isPaused && !m_HoveredToken.empty() && DebugSettings::ref()->isHoverEvalEnabled()) {
                    const double stillTime = ImGui::GetTime() - m_MouseStillSince;
                    if (stillTime >= AppSettings::ref()->getHoverDelaySec()) {
                        if (m_HoveredToken != m_LastHoverEvalToken) {
                            ++m_HoverEvalId;  // monotonic — never re-used so a stale result never aliases
                            m_LastHoverEvalToken = m_HoveredToken;
                        }
                        Ltg::EvalResult result;
                        if (ScriptDebugger::ref()->getEvalResult(m_HoverEvalId, result)) {
                            if (result.error.empty()) {
                                ImGui::SetTooltip("%s : %s", result.value.c_str(), result.typeName.c_str());
                            } else {
                                ImGui::SetTooltip("eval error: %s", result.error.c_str());
                            }
                        } else {
                            ScriptDebugger::ref()->requestEval(m_HoverEvalId, m_HoveredToken);
                            ImGui::SetTooltip("evaluating...");
                        }
                    }
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

    // toolbar laid out with ImGui::BeginLayoutHorizontal (standalone copy of ImNodal's primitives
    // in ImWidgets, decoupled from the ImNodal context). LayoutSpring() between the actions block
    // and the quick-show block pushes the latter to the right edge. inside Begin/EndLayoutHorizontal
    // the per-widget SameLine() calls are dropped — items are auto-placed on the same line.
    ImGui::BeginLayoutHorizontal("##codepane_toolbar");

    ImGui::LayoutSpring();

    bool armed = ScriptDebugger::ref()->isDebugArmed();
    if (ImGui::ToggleContrastedButton(ICON_FONT_BUG " Debug (on)", ICON_FONT_BUG " Debug (off)", &armed, "Arm the script debugger")) {
        ScriptDebugger::ref()->setDebugArmed(armed);
    }

    ImGui::LayoutSpring();

    // Run / Continue — Run gated on `armed`, Continue gated on `isPaused` so the user can resume
    // out of a pause (incl. auto-bp-on-error) without having to flip the master Debug toggle.
    ImGui::BeginDisabled(isPaused ? false : (!armed || workerBusy));
    if (ImGui::ContrastedButton(ICON_FONT_PLAY "##dbg_run", isPaused ? "Continue" : "Run (analyse the log files)")) {
        if (isPaused) {
            ScriptDebugger::ref()->doContinue();
        } else if (!workerBusy) {
            m_StartAnalyse();
        }
    }
    ImGui::EndDisabled();

    ImGui::LayoutSpring();

    // Step* gated on `isPaused` only — meaningful whenever the worker is paused, regardless of
    // master Debug. The line hook is installed when shouldArmDebug() is true (which includes the
    // auto-bp-on-error case), so step actually does something even when the master toggle is off.
    ImGui::BeginDisabled(!isPaused);
    if (ImGui::ContrastedButton(ICON_FONT_DEBUG_STEP_INTO "##dbg_stepinto", "Step into")) {
        ScriptDebugger::ref()->stepInto();
    }
    ImGui::EndDisabled();

    ImGui::LayoutSpring();

    ImGui::BeginDisabled(!isPaused);
    if (ImGui::ContrastedButton(ICON_FONT_DEBUG_STEP_OVER "##dbg_stepover", "Step over")) {
        ScriptDebugger::ref()->stepOver();
    }
    ImGui::EndDisabled();

    ImGui::LayoutSpring();

    ImGui::BeginDisabled(!isPaused);
    if (ImGui::ContrastedButton(ICON_FONT_DEBUG_STEP_OUT "##dbg_stepout", "Step out")) {
        ScriptDebugger::ref()->stepOut();
    }
    ImGui::EndDisabled();

    ImGui::LayoutSpring();

    ImGui::BeginDisabled(!armed || !workerBusy || isPaused);
    if (ImGui::ContrastedButton(ICON_FONT_PAUSE "##dbg_pause", "Pause")) {
        ScriptDebugger::ref()->pause();
    }
    ImGui::EndDisabled();

    ImGui::LayoutSpring();

    // Stop is available whenever the worker is busy (running OR paused), regardless of master
    // Debug state — you can always abort a run you started, incl. an auto-bp-on-error pause where
    // master Debug is off. Without this the user could be stuck "Continue"-ing forever through
    // the rest of the file.
    ImGui::BeginDisabled(!workerBusy);
    if (ImGui::ContrastedButton(ICON_FONT_STOP "##dbg_stop", "Stop")) {
        ScriptingEngine::s_working = false;  // break the parse loop on the worker thread
        ScriptDebugger::ref()->stop();
    }
    ImGui::EndDisabled();

    ImGui::LayoutSpring();

    if (isPaused) {
        // m_StateCache filled at the top of drawPanes (same frame), no need to re-query getState()
        ImGui::Text("Paused | log row %d | line %d", m_StateCache.logRowIndex, m_StateCache.line);
    } else if (workerBusy) {
        ImGui::TextUnformatted("Running");
    } else {
        ImGui::TextUnformatted("Idle");
    }

    ImGui::LayoutSpring(1.0f);

    ImGui::Text("%s", "Panes");

    ImGui::LayoutSpring();

    // quick-show buttons for the debug-related panes (no toggle: pane's own X closes it).
    // each call shows AND focuses so the pane lands on top even if it was already in the layout.
    if (ImGui::ContrastedButton(ICON_FONT_BUG "##sh_bp", "Show Breakpoints pane")) {
        ImLayout::ref().showAndFocusSpecificPane(BreakpointsPane::ref()->getFlag());
    }

    ImGui::LayoutSpring();

    if (ImGui::ContrastedButton(ICON_FONT_FORMAT_LIST_BULLETED "##sh_ct", "Show Call Trace pane")) {
        ImLayout::ref().showAndFocusSpecificPane(CalltracePane::ref()->getFlag());
    }

    ImGui::LayoutSpring();

    if (ImGui::ContrastedButton(ICON_FONT_FILE_TREE "##sh_st", "Show Stack Tree pane")) {
        ImLayout::ref().showAndFocusSpecificPane(StackTreePane::ref()->getFlag());
    }

    ImGui::LayoutSpring();

    if (ImGui::ContrastedButton(ICON_FONT_CROSSHAIRS "##sh_sc", "Show Scope pane")) {
        ImLayout::ref().showAndFocusSpecificPane(ScopePane::ref()->getFlag());
    }

    ImGui::LayoutSpring();

    if (ImGui::ContrastedButton(ICON_FONT_EYE "##sh_wt", "Show Watcher pane")) {
        ImLayout::ref().showAndFocusSpecificPane(WatcherPane::ref()->getFlag());
    }

    ImGui::EndLayoutHorizontal();

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
        // ImCode lexer names — "cpp"/"c" share the C/C++ lexer, "lua"/"glsl"/"sql" have their own
        CodeEditorLanguage type = "cpp";
        if (ps.ext == "cpp" || ps.ext == "hpp") {
            type = "cpp";
        } else if (ps.ext == "c" || ps.ext == "h") {
            type = "c";
        } else if (ps.ext == "lua") {
            type = "lua";
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
                ProjectFile::ref()->SetProjectChange(true);  // user gesture -> project dirty so it gets saved
            });
            sheet.codeEditor.SetTokenContextCallback([](const std::string& aToken) { WatcherPane::ref()->AddExpression(aToken); });
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
            ProjectFile::ref()->SetProjectChange(true);  // user gesture -> project dirty so it gets saved
        });
        sheet.codeEditor.SetTokenContextCallback([](const std::string& aToken) { WatcherPane::ref()->AddExpression(aToken); });
        // hover-eval — the editor reports the token under the mouse every frame; we stash it in
        // the CodePane and the post-tabbar code (drawPanes) drives the eval + tooltip.
        sheet.codeEditor.SetHoverTokenCallback([](const std::string& aToken) { CodePane::ref()->m_HoveredToken = aToken; });
        // editor Ctrl+S (or its File > Save) persists the project script into the .ltg db
        sheet.codeEditor.SetSaveCallback([]() { ProjectFile::ref()->Save(); });
        scriptSheetPtr = &sheet;
    }
    scriptSheetPtr->codeEditor.SetCode(aCode, "lua");
    // restore the persisted zoom — one-shot pending value that the editor applies on its next
    // Render after BeginChild. After that, ImGui's interactive Ctrl+MouseWheel takes over and the
    // drawPanes loop above captures any change back into ProjectFile.
    scriptSheetPtr->codeEditor.SetPendingFontScale(ProjectFile::ref()->m_ProjectScriptFontScale);
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

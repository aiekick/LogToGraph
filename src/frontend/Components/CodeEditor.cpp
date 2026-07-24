#include "CodeEditor.h"
#include "CodeUtils.h"
#include <ezlibs/ezTools.hpp>
#include <models/script/ScriptingEngine.h>
#include <settings/DebugSettings.h>

#include <cstdio>
#include <fstream>

// The editor widget is im::Code (ImCode v0.2). ImCode owns the canvas AND the IDE overlay
// UIs (completion popup, signature tooltip, context menus, hover, local zoom); this host
// class owns the FEATURE LOGIC on top: the completion catalog + filter state machine, the
// signature arg-index byte-scan, the breakpoint/error marker sets, and the pane chrome
// (menus, go-to-line, find popup, Ctrl+S/R shortcuts).
// Coordinates are BYTE columns everywhere (ImCode convention).

namespace {

void splitLines(const std::string& aText, std::vector<std::string>& aoLines) {
    aoLines.clear();
    size_t start = 0;
    while (true) {
        const size_t pos = aText.find('\n', start);
        if (pos == std::string::npos) {
            aoLines.push_back(aText.substr(start));
            break;
        }
        aoLines.push_back(aText.substr(start, pos - start));
        start = pos + 1;
    }
}

}  // namespace

bool CodeEditor::init() {
    if (ImGui::GetIO().Fonts->Fonts.size() > 1) {
        m_CodeFontPtr = ImGui::GetIO().Fonts->Fonts[1];
    }
    im::Code::Config config;
    config.flags = im::Code::Flags_ShowLineNumbers | im::Code::Flags_ShowGutter | im::Code::Flags_HighlightCurLine;
    m_Editor.init(config);
    m_ApplyPalette(true);
    // the mono font is pushed by ImCode itself; its internal Ctrl+MouseWheel zoom scales it
    m_Editor.getStyle().font = m_CodeFontPtr;

    // left click on the gutter toggles a breakpoint on that line (widget 0-based).
    // Toggling is ALWAYS allowed — breakpoints set while Debug is off are stored dormant
    // (they fire once shouldArmDebug() installs the line hook); the marker alpha fades as
    // a hint — see m_BreakpointInteractionEnabled in m_RebuildMarkers.
    m_Editor.setGutterClickCallback([this](int32_t aLine) {
        const bool has = (m_BreakpointLines.find(aLine) != m_BreakpointLines.end());
        if (m_OnBreakpointToggled) {
            m_OnBreakpointToggled(aLine, !has);
        }
    });
    // right-click on the gutter: breakpoint set/remove menu (rendered inside ImCode's popup).
    // Items are disabled when debug is off (dormant breakpoints CAN still be toggled with a
    // plain gutter click).
    m_Editor.setGutterContextMenuCallback([this](int32_t aLine) {
        const bool hasBreakpoint = (m_BreakpointLines.find(aLine) != m_BreakpointLines.end());
        ImGui::BeginDisabled(!m_BreakpointInteractionEnabled);
        if (!hasBreakpoint) {
            if (ImGui::MenuItem("Set Breakpoint")) {
                if (m_OnBreakpointToggled) {
                    m_OnBreakpointToggled(aLine, true);
                }
            }
        } else {
            if (ImGui::MenuItem("Remove Breakpoint")) {
                if (m_OnBreakpointToggled) {
                    m_OnBreakpointToggled(aLine, false);
                }
            }
        }
        ImGui::EndDisabled();
    });
    // right-click on the text: "Watch <token>" when the click lands on an identifier
    // (rendered inside ImCode's popup — an empty menu means the click was on punctuation)
    m_Editor.setTextContextMenuCallback([this](int32_t aLine, int32_t aColumn) {
        if (!m_OnTokenContext) {
            return;
        }
        const std::string token = m_ExtractTokenAt(aLine, aColumn);
        if (token.empty()) {
            return;
        }
        const std::string label = std::string("Watch \"") + token + "\"";
        if (ImGui::MenuItem(label.c_str())) {
            m_OnTokenContext(token);
        }
    });
    // per-frame mouse hover over text → propagate the token under the mouse to the consumer.
    // ALWAYS fired (even with an empty token on whitespace/punctuation) so the consumer can
    // detect when the user has moved off a previously-hovered identifier.
    m_Editor.setTextHoverCallback([this](int32_t aLine, int32_t aColumn) {
        if (m_OnHoverToken) {
            m_OnHoverToken(m_ExtractTokenAt(aLine, aColumn));
        }
    });
    // characters typed in the editor drive the autocompletion/signature state machines
    m_Editor.setCharacterTypedCallback([this](unsigned int aChar, int32_t aLine, int32_t aColumn) {
        m_OnCharacterTyped(aChar, aLine, aColumn);
    });
    // ImCode owns the popup rendering + the Up/Down/Enter/Escape interception; it routes
    // back here when the user picks an entry or dismisses the popup
    m_Editor.setCompletionCallbacks(
        [this](size_t aSelectedIndex) { m_OnCompletionAccepted(aSelectedIndex); },
        [this]() { m_OnCompletionCancelled(); });

    m_RefreshTextCache();
    return true;
}

void CodeEditor::unit() {
    m_Editor.unit();
}

void CodeEditor::OnImGui() {
    ImGuiIO& io = ImGui::GetIO();
    m_RefreshTextCache();

    bool requestingGoToLinePopup = false;
    bool requestingFindPopup = false;
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("Edit")) {
            // ImCode does not expose can-undo/can-redo — items stay enabled (no-op when empty)
            if (ImGui::MenuItem("Undo", "Ctrl+Z")) {
                m_Editor.execute(im::Code::Command::Undo);
            }
            if (ImGui::MenuItem("Redo", "Ctrl+Y")) {
                m_Editor.execute(im::Code::Command::Redo);
            }

            ImGui::Separator();

            const auto selection = m_Editor.getSelection();
            const bool hasSelection = (selection.start.line != selection.end.line) || (selection.start.column != selection.end.column);
            if (ImGui::MenuItem("Copy", "Ctrl+C", nullptr, hasSelection)) {
                m_Editor.execute(im::Code::Command::Copy);
            }
            if (ImGui::MenuItem("Cut", "Ctrl+X", nullptr, hasSelection)) {
                m_Editor.execute(im::Code::Command::Cut);
            }
            if (ImGui::MenuItem("Paste", "Ctrl+V", nullptr, ImGui::GetClipboardText() != nullptr)) {
                m_Editor.execute(im::Code::Command::Paste);
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Select all", "Ctrl+A", nullptr)) {
                m_Editor.execute(im::Code::Command::SelectAll);
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Find")) {
            if (ImGui::MenuItem("Go to line", "Ctrl+G")) {
                requestingGoToLinePopup = true;
            }
            // Ctrl+F focused in the editor opens ImCode's own find/replace bar; this menu
            // entry opens the host popup (works even when the editor child has no focus)
            if (ImGui::MenuItem("Find")) {
                requestingFindPopup = true;
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Palette")) {
            if (ImGui::MenuItem("Dark")) {
                m_ApplyPalette(true);
            }
            if (ImGui::MenuItem("Light")) {
                m_ApplyPalette(false);
            }
            ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
    }

    // one-shot restore of the persisted zoom (project load) — ImCode's zoom is
    // readable/settable, interactive Ctrl+MouseWheel is handled internally
    if (m_PendingFontScale > 0.0f) {
        m_Editor.setZoom(m_PendingFontScale);
        m_PendingFontScale = 0.0f;
    }

    // completion-open state BEFORE Render: when ImCode consumes an Escape press to close
    // its popup, the SAME press must not also close the signature tooltip below
    const bool completionWasOpen = m_Editor.isCompletionPopupOpen();

    m_Editor.Render("ltg_code_editor", ImVec2(0.0f, 0.0f));

    m_RefreshTextCache();  // pick up the edits made inside Render so every scan below works on fresh lines

    // signature-help: source of truth is the bytes between the opening `(` and the caret.
    // Recomputing every frame keeps depth + arg index in sync with Backspace, Delete,
    // paste, and arrow-key edits. Then Escape gets a chance to actively close the tooltip.
    m_RecomputeSignatureState();
    const bool escapeConsumedByCompletion = completionWasOpen && !m_Editor.isCompletionPopupOpen() && ImGui::IsKeyPressed(ImGuiKey_Escape, false);
    if (!escapeConsumedByCompletion) {
        m_HandleSignatureDismissKeys();
    }

    // pane-level shortcuts — the editor child owns the keyboard once clicked, so the scope
    // is "this window or any of its children". Ctrl+F is NOT handled here on purpose:
    // with the editor focused ImCode opens its own (richer) find/replace bar.
    const bool anyFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows);
    if (anyFocused && io.KeyCtrl) {
        if (ImGui::IsKeyPressed(ImGuiKey_S, false)) {
            OnSaveCommand();
        }
        if (ImGui::IsKeyPressed(ImGuiKey_R, false)) {
            OnReloadCommand();
        }
        if (ImGui::IsKeyPressed(ImGuiKey_G, false)) {
            requestingGoToLinePopup = true;
        }
    }

    if (requestingGoToLinePopup) {
        ImGui::OpenPopup("go_to_line_popup");
    }
    if (ImGui::BeginPopup("go_to_line_popup")) {
        static int targetLine;
        ImGui::SetKeyboardFocusHere();
        ImGui::InputInt("Line", &targetLine);
        if (ImGui::IsKeyDown(ImGuiKey_Enter) || ImGui::IsKeyDown(ImGuiKey_KeypadEnter)) {
            const int32_t targetLineFixed = (targetLine < 1) ? 0 : (targetLine - 1);
            const int32_t lineLen = (int32_t)m_GetLineText(targetLineFixed).size();
            m_Editor.setSelection(im::Code::Range{im::Code::Pos{targetLineFixed, 0}, im::Code::Pos{targetLineFixed, lineLen}});
            ImGui::CloseCurrentPopup();
            ImGui::GetIO().ClearInputKeys();
        } else if (ImGui::IsKeyDown(ImGuiKey_Escape)) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (requestingFindPopup) {
        ImGui::OpenPopup("find_popup");
    }
    if (ImGui::BeginPopup("find_popup")) {
        ImGui::Checkbox("Case sensitive", &m_CtrlfCaseSensitive);
        if (requestingFindPopup) {
            ImGui::SetKeyboardFocusHere();
        }
        ImGui::InputText("To find", m_CtrlfTextToFind, FIND_POPUP_TEXT_FIELD_LENGTH, ImGuiInputTextFlags_AutoSelectAll);
        const int32_t toFindTextSize = (int32_t)strlen(m_CtrlfTextToFind);
        const im::Code::FindFlags findFlags = m_CtrlfCaseSensitive ? im::Code::FindFlags_None : im::Code::FindFlags_CaseInsensitive;
        if ((ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter)) && toFindTextSize > 0) {
            m_Editor.setSearch(m_CtrlfTextToFind, findFlags);
            m_Editor.findNext();  // selects the next match after the caret (wraps)
        }
        if (ImGui::Button("Find all") && toFindTextSize > 0) {
            m_Editor.setSearch(m_CtrlfTextToFind, findFlags);  // every match stays highlighted by ImCode
        } else if (ImGui::IsKeyDown(ImGuiKey_Escape)) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void CodeEditor::SetSelection(int startLine, int startChar, int endLine, int endChar) {
    m_Editor.setSelection(im::Code::Range{im::Code::Pos{startLine, startChar}, im::Code::Pos{endLine, endChar}});
}

void CodeEditor::SetRelatedFile(const std::string& vFile) {
    m_RelatedFile = vFile;
}

const std::string& CodeEditor::GetRelatedFile() {
    return m_RelatedFile;
}

void CodeEditor::OnFolderViewDeleted(int folderViewId) {
    if (m_CreatedFromFolderView == folderViewId) {
        m_CreatedFromFolderView = -1;
    }
}

void CodeEditor::SetShowDebugPanel(bool value) {
    m_ShowDebugPanel = value;
}

void CodeEditor::SetCode(const std::string& vCode, CodeEditorLanguage vType) {
    m_Type = vType;
    m_Editor.setLanguage((m_Type != nullptr) ? m_Type : "");
    m_Editor.setText(vCode.data(), (uint64_t)vCode.size());
    m_RefreshTextCache();
    m_SavedTextHash = m_TextHash;  // freshly set text == on-disk state
}

std::string CodeEditor::GetCode() const {
    return m_Editor.getText();
}

bool CodeEditor::IsModified() const {
    return m_TextHash != m_SavedTextHash;
}

size_t CodeEditor::GetUndoIndex() const {
    // monotonic edit counter — ImCode exposes no undo index, so the host bumps m_Revision
    // on every observed content change (see m_RefreshTextCache). Same contract for the
    // consumers: "the value changed => an edit happened".
    return m_Revision;
}

void CodeEditor::MarkSaved() {
    m_SavedTextHash = m_TextHash;
}

void CodeEditor::ClearErrorMarkers() {
    m_ErrorMarkers.clear();
    m_RebuildMarkers();
}

void CodeEditor::AddErrorMarker(const size_t& vErrorLine, const std::string& vErrorMsg) {
    // the error is surfaced as an ImCode diagnostic: red squiggle over the line + the
    // message as hover tooltip. The caret is NOT moved here — the host drives caret
    // movement exclusively via MoveCursorTo on state-revision bumps.
    m_ErrorMarkers[(int32_t)vErrorLine] = vErrorMsg;
    m_RebuildMarkers();
}

// Commands

void CodeEditor::OnReloadCommand() {
#if defined(__WIN32__) || defined(WIN32) || defined(_WIN32) || defined(__WIN64__) || defined(WIN64) || defined(_WIN64) || defined(_MSC_VER)
    std::ifstream t(ez::str::utf8Decode(m_RelatedFile).c_str());
#else
    std::ifstream t(m_RelatedFile);
#endif
    std::string str((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
    m_Editor.setText(str.data(), (uint64_t)str.size());
    m_RefreshTextCache();
    m_SavedTextHash = m_TextHash;
}

void CodeEditor::OnLoadFromCommand() {}

void CodeEditor::OnSaveCommand() {
    if (m_OnSave) {
        m_OnSave();
    }
}

void CodeEditor::SetSaveCallback(std::function<void()> aCallback) {
    m_OnSave = aCallback;
}

void CodeEditor::SetBreakpointToggledCallback(std::function<void(int32_t, bool)> aCallback) {
    m_OnBreakpointToggled = aCallback;
}

void CodeEditor::SetTokenContextCallback(std::function<void(const std::string&)> aCallback) {
    m_OnTokenContext = aCallback;
}

void CodeEditor::SetHoverTokenCallback(std::function<void(const std::string&)> aCallback) {
    m_OnHoverToken = aCallback;
}

void CodeEditor::SetBreakpoints(const std::unordered_set<int32_t>& aZeroBasedLines, int64_t aRevision) {
    if (aRevision == m_LastBreakpointsRevision) {
        return;  // no change since last call — skip the set copy + marker rebuild
    }
    m_LastBreakpointsRevision = aRevision;
    m_BreakpointLines = aZeroBasedLines;
    m_RebuildMarkers();
}

void CodeEditor::SetCurrentExecLine(int32_t aZeroBasedLine) {
    if (m_CurrentExecLine != aZeroBasedLine) {
        m_CurrentExecLine = aZeroBasedLine;
        m_RebuildMarkers();
    }
}

void CodeEditor::MoveCursorTo(int32_t aZeroBasedLine, int32_t aZeroBasedColumn) {
    // explicit caret jump — used by CodePane to sync the caret to the active line on each new
    // pause event (state revision bump). Separated from SetCurrentExecLine so the marker update
    // doesn't fight a user who manually moved the caret between pauses.
    if (aZeroBasedLine >= 0) {
        m_Editor.setCursor(im::Code::Pos{aZeroBasedLine, aZeroBasedColumn});
    }
}

void CodeEditor::SetBreakpointInteractionEnabled(bool aEnabled) {
    if (m_BreakpointInteractionEnabled != aEnabled) {
        m_BreakpointInteractionEnabled = aEnabled;
        m_RebuildMarkers();  // the marker alpha encodes the armed/dormant state
    }
}

void CodeEditor::SetPendingFontScale(float aScale) {
    m_PendingFontScale = aScale;
}

float CodeEditor::GetCurrentFontScale() const {
    return m_Editor.getZoom();
}

void CodeEditor::m_RebuildMarkers() {
    // gutter dots: breakpoints (red) then the current exec line (amber) — later markers on
    // the same line draw on top, so a pause on a breakpoint shows the amber dot over the red
    // one while both hover messages stay reachable through the gutter tooltip.
    std::vector<im::Code::Marker> markers;
    markers.reserve(m_BreakpointLines.size() + 1);
    // when debug is off the user can still toggle breakpoints but they won't fire yet —
    // fade the dot alpha as a visual hint.
    const ImU32 breakpointAlpha = m_BreakpointInteractionEnabled ? 255 : 110;
    for (const auto& breakpointLine : m_BreakpointLines) {
        markers.push_back({breakpointLine, IM_COL32(220, 40, 40, breakpointAlpha), "breakpoint", 1});
    }
    if (m_CurrentExecLine >= 0) {
        markers.push_back({m_CurrentExecLine, IM_COL32(255, 200, 0, 255), "current line", 2});
    }
    m_Editor.setMarkers(markers);

    // errors: one diagnostic per line — red squiggle over the whole line + message on hover
    std::vector<im::Code::Diagnostic> diagnostics;
    diagnostics.reserve(m_ErrorMarkers.size());
    for (const auto& errorMarker : m_ErrorMarkers) {
        const int32_t errorLine0Based = errorMarker.first - 1;
        if (errorLine0Based < 0) {
            continue;
        }
        int32_t lineLen = (int32_t)m_GetLineText(errorLine0Based).size();
        if (lineLen < 1) {
            lineLen = 1;  // keep a visible squiggle span on empty lines
        }
        diagnostics.push_back({im::Code::Range{im::Code::Pos{errorLine0Based, 0}, im::Code::Pos{errorLine0Based, lineLen}},
                               im::Code::Severity::Error,
                               errorMarker.second});
    }
    m_Editor.setDiagnostics(diagnostics);

    // current paused line: amber underline across the line (the marker dot alone is easy to
    // miss when the gutter also holds a breakpoint dot)
    std::vector<im::Code::Decoration> decorations;
    if (m_CurrentExecLine >= 0) {
        int32_t lineLen = (int32_t)m_GetLineText(m_CurrentExecLine).size();
        if (lineLen < 1) {
            lineLen = 1;
        }
        decorations.push_back({im::Code::Range{im::Code::Pos{m_CurrentExecLine, 0}, im::Code::Pos{m_CurrentExecLine, lineLen}},
                               im::Code::DecoKind::Underline,
                               IM_COL32(255, 200, 0, 200),
                               "current line"});
    }
    m_Editor.setDecorations(decorations);
}

void CodeEditor::m_ApplyPalette(bool aDark) {
    im::Code::Style& style = m_Editor.getStyle();
    ImU32* colors = style.colors;
    if (aDark) {
        // mirrors ImCode's builtin dark defaults
        colors[im::Code::Col_Background] = IM_COL32(30, 30, 30, 255);
        colors[im::Code::Col_Default] = IM_COL32(220, 220, 220, 255);
        colors[im::Code::Col_LineNumber] = IM_COL32(120, 120, 120, 255);
        colors[im::Code::Col_Gutter] = IM_COL32(24, 24, 24, 255);
        colors[im::Code::Col_CurrentLine] = IM_COL32(42, 42, 42, 255);
        colors[im::Code::Col_Selection] = IM_COL32(60, 90, 140, 120);
        colors[im::Code::Col_Caret] = IM_COL32(230, 230, 230, 255);
        colors[im::Code::Col_Keyword] = IM_COL32(86, 156, 214, 255);
        colors[im::Code::Col_Type] = IM_COL32(78, 201, 176, 255);
        colors[im::Code::Col_Identifier] = IM_COL32(220, 220, 220, 255);
        colors[im::Code::Col_String] = IM_COL32(214, 157, 133, 255);
        colors[im::Code::Col_Char] = IM_COL32(214, 157, 133, 255);
        colors[im::Code::Col_Number] = IM_COL32(181, 206, 168, 255);
        colors[im::Code::Col_Comment] = IM_COL32(106, 153, 85, 255);
        colors[im::Code::Col_Preproc] = IM_COL32(155, 155, 155, 255);
        colors[im::Code::Col_Operator] = IM_COL32(200, 200, 200, 255);
        colors[im::Code::Col_Punctuation] = IM_COL32(200, 200, 200, 255);
        colors[im::Code::Col_SearchMatch] = IM_COL32(130, 100, 40, 140);
        colors[im::Code::Col_MarkerError] = IM_COL32(220, 80, 80, 255);
        colors[im::Code::Col_MarkerWarning] = IM_COL32(220, 180, 60, 255);
        colors[im::Code::Col_FoldMarker] = IM_COL32(120, 120, 120, 255);
        colors[im::Code::Col_Whitespace] = IM_COL32(80, 80, 80, 255);
        colors[im::Code::Col_IndentGuide] = IM_COL32(60, 60, 60, 255);
        colors[im::Code::Col_Ruler] = IM_COL32(60, 60, 60, 255);
        colors[im::Code::Col_Minimap] = IM_COL32(120, 120, 120, 120);
        colors[im::Code::Col_BracketMatch] = IM_COL32(120, 160, 220, 120);
    } else {
        colors[im::Code::Col_Background] = IM_COL32(250, 250, 250, 255);
        colors[im::Code::Col_Default] = IM_COL32(30, 30, 30, 255);
        colors[im::Code::Col_LineNumber] = IM_COL32(140, 140, 140, 255);
        colors[im::Code::Col_Gutter] = IM_COL32(238, 238, 238, 255);
        colors[im::Code::Col_CurrentLine] = IM_COL32(232, 232, 236, 255);
        colors[im::Code::Col_Selection] = IM_COL32(160, 190, 230, 120);
        colors[im::Code::Col_Caret] = IM_COL32(20, 20, 20, 255);
        colors[im::Code::Col_Keyword] = IM_COL32(0, 0, 255, 255);
        colors[im::Code::Col_Type] = IM_COL32(43, 145, 175, 255);
        colors[im::Code::Col_Identifier] = IM_COL32(30, 30, 30, 255);
        colors[im::Code::Col_String] = IM_COL32(163, 21, 21, 255);
        colors[im::Code::Col_Char] = IM_COL32(163, 21, 21, 255);
        colors[im::Code::Col_Number] = IM_COL32(9, 134, 88, 255);
        colors[im::Code::Col_Comment] = IM_COL32(0, 128, 0, 255);
        colors[im::Code::Col_Preproc] = IM_COL32(128, 128, 128, 255);
        colors[im::Code::Col_Operator] = IM_COL32(60, 60, 60, 255);
        colors[im::Code::Col_Punctuation] = IM_COL32(60, 60, 60, 255);
        colors[im::Code::Col_SearchMatch] = IM_COL32(255, 220, 120, 160);
        colors[im::Code::Col_MarkerError] = IM_COL32(200, 40, 40, 255);
        colors[im::Code::Col_MarkerWarning] = IM_COL32(180, 130, 20, 255);
        colors[im::Code::Col_FoldMarker] = IM_COL32(140, 140, 140, 255);
        colors[im::Code::Col_Whitespace] = IM_COL32(190, 190, 190, 255);
        colors[im::Code::Col_IndentGuide] = IM_COL32(210, 210, 210, 255);
        colors[im::Code::Col_Ruler] = IM_COL32(210, 210, 210, 255);
        colors[im::Code::Col_Minimap] = IM_COL32(140, 140, 140, 120);
        colors[im::Code::Col_BracketMatch] = IM_COL32(120, 160, 220, 120);
    }
}

///////////////////////////////////////////////////////////////////////////////////
//// TEXT MIRROR //////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////

void CodeEditor::m_RefreshTextCache() {
    // per-frame content watch: hash the text and re-split the line cache when it changed.
    // Any observed change bumps m_Revision (the host-side "undo index" / edit counter).
    const std::string text = m_Editor.getText();
    const size_t hash = std::hash<std::string>{}(text);
    if (hash == m_TextHash && !m_CachedLines.empty()) {
        return;
    }
    m_TextHash = hash;
    ++m_Revision;
    splitLines(text, m_CachedLines);
}

const std::string& CodeEditor::m_GetLineText(int32_t aLine) const {
    static const std::string s_Empty;
    if (aLine < 0 || (size_t)aLine >= m_CachedLines.size()) {
        return s_Empty;
    }
    return m_CachedLines[(size_t)aLine];
}

std::string CodeEditor::m_ExtractTokenAt(int aLine, int aByteColumn) {
    return ltg::code::extractTokenAt(m_GetLineText(aLine), aByteColumn);
}

///////////////////////////////////////////////////////////////////////////////////
//// AUTOCOMPLETION (state machine — ImCode owns the popup UI) ////////////////////
///////////////////////////////////////////////////////////////////////////////////

void CodeEditor::m_OnCharacterTyped(unsigned int aCharacter, int aLine, int aColumn) {
    // fired from INSIDE ImCode::Render, right after the insertion: refresh the line cache
    // first so the token scans below see the just-typed characters
    m_RefreshTextCache();

    // === completion popup: filter extension / dismissal =====================================
    if (m_Editor.isCompletionPopupOpen()) {
        if (ltg::code::isIdentChar((char)aCharacter)) {
            m_CompletionFilter += static_cast<char>(aCharacter);
            m_RecomputeCompletionFiltered();  // closes the popup when the filter matches nothing
            return;
        }
        m_OnCompletionCancelled();
        // fall through — a non-ident char may also be a `(` that triggers the signature
    } else if ((aCharacter == '.' || aCharacter == ':') && DebugSettings::ref()->isAutoCompletionEnabled()) {
        // === completion trigger: `.` or `:` after a known catalog key ======================
        // `aColumn` is the BYTE cursor RIGHT AFTER the trigger char. The identifier we want
        // is to the LEFT of the trigger (the trigger itself is at column - 1). gated by
        // DebugSettings — when off, no popup ever opens.
        const std::string target = m_ExtractTokenAt(aLine, aColumn - 2);
        if (target.empty()) {
            return;
        }
        std::vector<Ltg::CompletionEntry> entries;
        ScriptingEngine::ref()->GetCompletionEntries(target, entries);
        if (entries.empty()) {
            return;
        }
        m_CompletionTarget = target;
        m_CompletionAllEntries = std::move(entries);
        m_CompletionFilter.clear();
        m_CompletionAnchorLine = aLine;
        m_CompletionAnchorColumn = aColumn;  // right after the trigger — where filter chars will start to land
        m_RecomputeCompletionFiltered();
        return;
    }

    // === signature trigger: `(` when no tooltip is currently open ==========================
    // when the tooltip is already open, the nested `(` was counted by the per-frame scan
    // (paren depth), and a nested signature tooltip would compete for screen space with no
    // clean UI to disambiguate. gated by DebugSettings.
    if (aCharacter == '(' && !m_Editor.isSignatureTooltipOpen() && DebugSettings::ref()->isSignatureHelpEnabled()) {
        std::string target;
        std::string functionName;
        if (ltg::code::extractCallTargetAt(m_GetLineText(aLine), aColumn - 1, target, functionName)) {
            m_OpenSignature(target, functionName);
        }
    }
}

void CodeEditor::m_RecomputeCompletionFiltered() {
    // case-sensitive prefix match — fits Lua identifier conventions
    std::vector<std::string> names;
    names.reserve(m_CompletionAllEntries.size());
    for (const auto& entry : m_CompletionAllEntries) {
        names.push_back(entry.name);
    }
    const std::vector<size_t> kept = ltg::code::filterByPrefix(names, m_CompletionFilter);
    m_CompletionFilteredEntries.clear();
    m_CompletionFilteredEntries.reserve(kept.size());
    std::vector<im::Code::CompletionItem> popupItems;
    popupItems.reserve(kept.size());
    for (const size_t index : kept) {
        const auto& entry = m_CompletionAllEntries[index];
        m_CompletionFilteredEntries.push_back(entry);
        im::Code::CompletionItem item;
        item.label = entry.name;
        item.insertText = entry.name;
        item.detail = entry.type;
        popupItems.push_back(std::move(item));
    }
    if (m_CompletionFilteredEntries.empty()) {
        m_OnCompletionCancelled();  // empty list closes the popup silently
    } else {
        m_Editor.openCompletionPopup(popupItems, im::Code::Pos{m_CompletionAnchorLine, m_CompletionAnchorColumn});
    }
}

void CodeEditor::m_OnCompletionAccepted(size_t aSelectedIndex) {
    if (aSelectedIndex >= m_CompletionFilteredEntries.size()) {
        m_OnCompletionCancelled();
        return;
    }
    const auto entry = m_CompletionFilteredEntries[aSelectedIndex];

    // select the filter region (from the anchor — right after the trigger — to the current
    // cursor) and replace it with the full entry name: im::Code's insertText replaces the
    // active selection as one undo transaction.
    const im::Code::Pos cursor = m_Editor.getCursor();
    m_Editor.setSelection(im::Code::Range{im::Code::Pos{m_CompletionAnchorLine, m_CompletionAnchorColumn}, cursor});
    m_Editor.insertText(entry.name.c_str());

    m_OnCompletionCancelled();  // share the state-reset path
}

void CodeEditor::m_OnCompletionCancelled() {
    m_CompletionTarget.clear();
    m_CompletionAllEntries.clear();
    m_CompletionFilteredEntries.clear();
    m_CompletionFilter.clear();
    m_CompletionAnchorLine = 0;
    m_CompletionAnchorColumn = 0;
    m_Editor.closeCompletionPopup();  // no-op when ImCode initiated the close itself
}

///////////////////////////////////////////////////////////////////////////////////
//// SIGNATURE HELP (state machine — ImCode owns the tooltip UI) //////////////////
///////////////////////////////////////////////////////////////////////////////////

void CodeEditor::m_OpenSignature(const std::string& aTarget, const std::string& aFunctionName) {
    Ltg::SignatureInfo signature;
    ScriptingEngine::ref()->GetSignatureInfo(aTarget, aFunctionName, signature);
    if (signature.label.empty()) {
        return;  // unknown call — silently no tooltip
    }
    // anchor: line + BYTE col RIGHT AFTER the `(` (where the scan range starts each frame)
    const im::Code::Pos cursor = m_Editor.getCursor();
    m_SignatureAnchorLine = cursor.line;
    m_SignatureAnchorColumn = cursor.column;
    m_SignatureArgIndex = 0;

    im::Code::SignatureTooltip tooltip;
    tooltip.label = signature.label;
    tooltip.args.reserve(signature.args.size());
    for (const auto& arg : signature.args) {
        tooltip.args.push_back({arg.name, arg.type});
    }
    tooltip.currentArgIndex = 0;
    m_Editor.openSignatureTooltip(tooltip, cursor);
}

void CodeEditor::m_CloseSignature() {
    m_Editor.closeSignatureTooltip();
    m_SignatureAnchorLine = 0;
    m_SignatureAnchorColumn = 0;
    m_SignatureArgIndex = 0;
}

void CodeEditor::m_RecomputeSignatureState() {
    // recomputed every frame: re-scans the bytes between the opening `(` (captured at open
    // time) and the current caret to derive depth + comma count. handles Backspace, Delete,
    // paste, and arrow-key edits naturally — the source of truth is always the actual text.
    if (!m_Editor.isSignatureTooltipOpen()) {
        return;
    }
    const im::Code::Pos cursor = m_Editor.getCursor();

    // close if the caret left the line of the opening `(`. v1 doesn't follow multi-line calls.
    if (cursor.line != m_SignatureAnchorLine) {
        m_CloseSignature();
        return;
    }
    // close if the caret moved BEFORE the opening `(` (user backspaced past it or moved left)
    if (cursor.column < m_SignatureAnchorColumn) {
        m_CloseSignature();
        return;
    }
    int32_t argIndex = 0;
    if (!ltg::code::computeSignatureArgIndex(m_GetLineText(cursor.line), (size_t)m_SignatureAnchorColumn, (size_t)cursor.column, argIndex)) {
        m_CloseSignature();  // caret moved past the matching `)` — call is finished
        return;
    }
    if (argIndex != m_SignatureArgIndex) {
        m_SignatureArgIndex = argIndex;
        m_Editor.updateSignatureCurrentArg(argIndex);
    }
}

void CodeEditor::m_HandleSignatureDismissKeys() {
    // Escape is the only event that actively closes the tooltip — everything else (caret
    // moves, mouse clicks, Backspace, paste) is reconciled by m_RecomputeSignatureState().
    if (!m_Editor.isSignatureTooltipOpen()) {
        return;
    }
    if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
        m_CloseSignature();
    }
}

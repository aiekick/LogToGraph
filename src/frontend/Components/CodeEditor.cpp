#include "CodeEditor.h"
#include "CodeUtils.h"
#include <3rdparty/imgui_docking/imgui_internal.h>  // FindWindowByName / GetKeyData / GImGui — layout mirror + popup key interception
#include <ezlibs/ezTools.hpp>
#include <models/script/ScriptingEngine.h>
#include <settings/DebugSettings.h>

#include <cstdio>
#include <fstream>

// The old ImGuiColorTextEdit-based TextEditor is gone from imguipack; im::Code (ImCode)
// replaces it. ImCode v0.1 is a lean model+canvas: it owns text/caret/undo/find and the
// gutter/marker/decoration rendering, but has NO completion popup, NO signature tooltip,
// NO context menus and NO hover/char callbacks. Those features live HERE now, host-side,
// over ImCode's public API. Two ImCode internals are mirrored to make that possible:
//  - the render layout (char width / line height / gutter width / child scroll) so screen
//    coordinates map to (line, byte-column) and back — see m_CaptureLayout;
//  - the byte-column coordinate system (ImCode positions are byte offsets, the old
//    TextEditor reported tab-expanded visual columns — every consumer got simpler).

namespace {

constexpr const char* sc_EditorChildId = "ltg_code_editor";
constexpr int32_t sc_TabWidth = 4;  // mirrors im::Code::Style::tabWidth default (never changed by the host)

// keys the completion popup owns while it is open: they must not reach ImCode::Render
// (Up/Down would move the caret, Enter would insert a newline). IsKeyPressed reads
// ImGuiKeyData, so blanking Down/DownDuration around Render hides the press from ImCode
// while the host (which reads the keys BEFORE Render) still saw it.
constexpr ImGuiKey sc_PopupOwnedKeys[] = {ImGuiKey_UpArrow, ImGuiKey_DownArrow, ImGuiKey_Enter, ImGuiKey_KeypadEnter};

struct KeyBackup {
    bool down = false;
    float downDuration = -1.0f;
    float downDurationPrev = -1.0f;
};
KeyBackup g_KeyBackups[IM_ARRAYSIZE(sc_PopupOwnedKeys)];

void pushKeySuppression() {
    for (int i = 0; i < IM_ARRAYSIZE(sc_PopupOwnedKeys); ++i) {
        ImGuiKeyData* keyData = ImGui::GetKeyData(sc_PopupOwnedKeys[i]);
        g_KeyBackups[i].down = keyData->Down;
        g_KeyBackups[i].downDuration = keyData->DownDuration;
        g_KeyBackups[i].downDurationPrev = keyData->DownDurationPrev;
        keyData->Down = false;
        keyData->DownDuration = -1.0f;
        keyData->DownDurationPrev = -1.0f;
    }
}

void popKeySuppression() {
    for (int i = 0; i < IM_ARRAYSIZE(sc_PopupOwnedKeys); ++i) {
        ImGuiKeyData* keyData = ImGui::GetKeyData(sc_PopupOwnedKeys[i]);
        keyData->Down = g_KeyBackups[i].down;
        keyData->DownDuration = g_KeyBackups[i].downDuration;
        keyData->DownDurationPrev = g_KeyBackups[i].downDurationPrev;
    }
}

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
            // ImCode does not expose can-undo/can-redo in v0.1 — items stay enabled (no-op when empty)
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

    // one-shot restore of the persisted zoom (project load)
    if (m_PendingFontScale > 0.0f) {
        m_FontScale = ImClamp(m_PendingFontScale, 0.3f, 6.0f);
        m_PendingFontScale = 0.0f;
    }
    // Ctrl+MouseWheel zoom — HOST-owned so it can be persisted in the project file. The
    // wheel event is eaten before Render so ImCode's internal zoom (not readable from
    // outside) never engages; the scale is applied through the PushFont size below.
    // Uses the previous frame's child rect: good enough at frame rate.
    if (m_Layout.valid && io.KeyCtrl && io.MouseWheel != 0.0f) {
        const ImVec2 mouse = ImGui::GetMousePos();
        if (mouse.x >= m_Layout.childPos.x && mouse.x < m_Layout.childPos.x + m_Layout.childSize.x &&  //
            mouse.y >= m_Layout.childPos.y && mouse.y < m_Layout.childPos.y + m_Layout.childSize.y) {
            m_FontScale = ImClamp(m_FontScale * ((io.MouseWheel > 0.0f) ? 1.1f : (1.0f / 1.1f)), 0.3f, 6.0f);
            io.MouseWheel = 0.0f;
        }
    }

    // completion popup keyboard/mouse handling happens BEFORE Render (the popup owns
    // Up/Down/Enter while open); if the popup was open at frame start those keys are
    // then hidden from ImCode during Render — including on the accept/cancel frame.
    const bool popupOwnedKeysThisFrame = m_CompletionPopupVisible;
    bool escapeConsumedByCompletion = false;
    if (m_CompletionPopupVisible) {
        m_HandleCompletionKeys();
        if (!m_CompletionPopupVisible && ImGui::IsKeyPressed(ImGuiKey_Escape, false)) {
            escapeConsumedByCompletion = true;  // don't let the SAME Escape press also close the signature tooltip
        }
    }

    // parent window identity — needed to reconstruct the editor child window's name
    // ("parent/childid_%08X", the BeginChild naming scheme) for the layout capture.
    ImGuiWindow* parentWindow = ImGui::GetCurrentWindow();
    const ImGuiID childId = parentWindow->GetID(sc_EditorChildId);

    // the host owns font AND size: im::Code::Style::font stays null, so ImCode renders with
    // whatever is pushed here. PushFont(nullptr, size) keeps the current font (size only).
    const float editorFontSize = ImGui::GetFontSize() * m_FontScale;
    ImGui::PushFont(m_CodeFontPtr, editorFontSize);
    // sample the exact metrics ImCode::Render derives internally (same formulas)
    const float fontSize = ImGui::GetFontSize();
    const float spacingRatio = ImGui::GetTextLineHeightWithSpacing() / fontSize;
    const float lineHeight = fontSize * spacingRatio;
    const float charWidth = ImGui::GetFont()->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, "X").x;

    if (popupOwnedKeysThisFrame) {
        pushKeySuppression();
    }
    m_Editor.Render(sc_EditorChildId, ImVec2(0.0f, 0.0f));
    if (popupOwnedKeysThisFrame) {
        popKeySuppression();
    }
    ImGui::PopFont();

    m_CaptureLayout(sc_EditorChildId, childId, parentWindow->Name, fontSize, lineHeight, charWidth);
    m_RefreshTextCache();  // pick up the edits made inside Render so every mapping below works on fresh lines

    // characters inserted by ImCode this frame drive the completion / signature state
    // machines. Same gates as ImCode's own input loop: child focused, no Ctrl chord,
    // printable ASCII. The reported position is the caret AFTER insertion.
    if (m_Layout.valid && m_Layout.focused && !io.KeyCtrl) {
        const im::Code::Pos cursor = m_Editor.getCursor();
        for (int i = 0; i < io.InputQueueCharacters.Size; ++i) {
            const ImWchar character = io.InputQueueCharacters[i];
            if (character >= 32 && character != 127 && character < 128) {
                m_OnCharacterTyped(character, cursor.line, cursor.column);
            }
        }
    }

    m_HandleMouseInteractions();
    m_DrawContextMenus();

    // signature-help: source of truth is the bytes between the opening `(` and the caret.
    // Recomputing every frame keeps depth + arg index in sync with Backspace, Delete,
    // paste, and arrow-key edits. Then Escape gets a chance to actively close the tooltip.
    m_RecomputeSignatureState();
    if (!escapeConsumedByCompletion) {
        m_HandleSignatureDismissKeys();
    }

    m_DrawCompletionPopup();
    m_DrawSignatureTooltip();

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
    return m_FontScale;
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
//// LAYOUT MIRROR (ImCode::Render formulas) //////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////

void CodeEditor::m_CaptureLayout(const char* aChildStrId, ImGuiID aChildId, const char* aParentName, float aFontSize, float aLineHeight, float aCharWidth) {
    m_Layout.valid = false;
    char childWindowName[512];
    ImFormatString(childWindowName, sizeof(childWindowName), "%s/%s_%08X", aParentName, aChildStrId, aChildId);
    ImGuiWindow* childWindow = ImGui::FindWindowByName(childWindowName);
    if (childWindow == nullptr) {
        return;
    }
    m_Layout.valid = true;
    m_Layout.focused = (GImGui->NavWindow == childWindow);
    m_Layout.childPos = childWindow->Pos;
    m_Layout.childSize = childWindow->Size;
    m_Layout.scroll = childWindow->Scroll;
    m_Layout.fontSize = aFontSize;
    m_Layout.lineHeight = aLineHeight;
    m_Layout.charWidth = aCharWidth;
    // same gutter formula as ImCode::Render: digits * charWidth + 3 * (charWidth / 2)
    int32_t digits = 1;
    for (int32_t n = (int32_t)m_CachedLines.size(); n >= 10; n /= 10) {
        ++digits;
    }
    m_Layout.gutterWidth = (float)digits * aCharWidth + aCharWidth * 1.5f;
}

int32_t CodeEditor::m_DisplayColumn(int32_t aLine, int32_t aByteColumn) const {
    const std::string& line = m_GetLineText(aLine);
    const int32_t end = (aByteColumn < (int32_t)line.size()) ? aByteColumn : (int32_t)line.size();
    int32_t displayColumn = 0;
    for (int32_t k = 0; k < end; ++k) {
        if (line[(size_t)k] == '\t') {
            displayColumn += sc_TabWidth - (displayColumn % sc_TabWidth);
        } else {
            ++displayColumn;
        }
    }
    if (aByteColumn > end) {
        displayColumn += aByteColumn - end;  // bytes past EOL count as 1 each
    }
    return displayColumn;
}

int32_t CodeEditor::m_ByteColumnFromDisplay(int32_t aLine, float aDisplayColumns) const {
    const std::string& line = m_GetLineText(aLine);
    int32_t displayColumn = 0;
    for (int32_t k = 0; k < (int32_t)line.size(); ++k) {
        const int32_t advance = (line[(size_t)k] == '\t') ? (sc_TabWidth - (displayColumn % sc_TabWidth)) : 1;
        if (aDisplayColumns < (float)displayColumn + (float)advance * 0.5f) {
            return k;
        }
        displayColumn += advance;
    }
    return (int32_t)line.size();
}

bool CodeEditor::m_ScreenToTextPos(const ImVec2& aScreen, int32_t& aoLine, int32_t& aoByteColumn) const {
    if (!m_Layout.valid || m_CachedLines.empty() || m_Layout.lineHeight <= 0.0f || m_Layout.charWidth <= 0.0f) {
        return false;
    }
    const ImVec2 contentOrigin(m_Layout.childPos.x - m_Layout.scroll.x, m_Layout.childPos.y - m_Layout.scroll.y);
    const float textOriginX = contentOrigin.x + m_Layout.gutterWidth;
    int32_t line = (int32_t)((aScreen.y - contentOrigin.y) / m_Layout.lineHeight);
    if (line < 0) {
        line = 0;
    }
    const int32_t lastLine = (int32_t)m_CachedLines.size() - 1;
    if (line > lastLine) {
        line = lastLine;
    }
    aoLine = line;
    aoByteColumn = m_ByteColumnFromDisplay(line, (aScreen.x - textOriginX) / m_Layout.charWidth);
    return true;
}

bool CodeEditor::m_TextPosToScreen(int32_t aLine, int32_t aByteColumn, ImVec2& aoScreen) const {
    if (!m_Layout.valid || m_Layout.lineHeight <= 0.0f || m_Layout.charWidth <= 0.0f) {
        return false;
    }
    const ImVec2 contentOrigin(m_Layout.childPos.x - m_Layout.scroll.x, m_Layout.childPos.y - m_Layout.scroll.y);
    aoScreen.x = contentOrigin.x + m_Layout.gutterWidth + (float)m_DisplayColumn(aLine, aByteColumn) * m_Layout.charWidth;
    aoScreen.y = contentOrigin.y + (float)aLine * m_Layout.lineHeight;
    return true;
}

///////////////////////////////////////////////////////////////////////////////////
//// MOUSE INTERACTIONS (hover token + right-click menus) /////////////////////////
///////////////////////////////////////////////////////////////////////////////////

void CodeEditor::m_HandleMouseInteractions() {
    if (!m_Layout.valid) {
        return;
    }
    const ImVec2 mouse = ImGui::GetMousePos();
    const bool inChild = (mouse.x >= m_Layout.childPos.x && mouse.x < m_Layout.childPos.x + m_Layout.childSize.x &&  //
                          mouse.y >= m_Layout.childPos.y && mouse.y < m_Layout.childPos.y + m_Layout.childSize.y);
    if (!inChild) {
        return;
    }
    // the gutter is drawn at a FIXED x (not scrolled) — same zone test as ImCode's click path
    const bool inGutter = (mouse.x < m_Layout.childPos.x + m_Layout.gutterWidth);
    if (inGutter) {
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
            const ImVec2 contentOrigin(m_Layout.childPos.x - m_Layout.scroll.x, m_Layout.childPos.y - m_Layout.scroll.y);
            const int32_t line = (int32_t)((mouse.y - contentOrigin.y) / m_Layout.lineHeight);
            if (line >= 0 && (size_t)line < m_CachedLines.size()) {
                m_GutterContextLine = line;
                ImGui::OpenPopup("ltg_code_gutter_ctx");
            }
        }
        return;
    }
    int32_t line = 0;
    int32_t byteColumn = 0;
    if (!m_ScreenToTextPos(mouse, line, byteColumn)) {
        return;
    }
    // per-frame hover — ALWAYS fired (even with an empty token on whitespace/punctuation)
    // so the consumer can detect when the user has moved off a previously-hovered identifier.
    if (m_OnHoverToken) {
        m_OnHoverToken(m_ExtractTokenAt(line, byteColumn));
    }
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Right) && m_OnTokenContext) {
        const std::string token = m_ExtractTokenAt(line, byteColumn);
        if (!token.empty()) {
            m_ContextToken = token;
            ImGui::OpenPopup("ltg_code_text_ctx");
        }
    }
}

void CodeEditor::m_DrawContextMenus() {
    // right-click on the gutter: breakpoint set/remove. Items are disabled when debug is
    // off (dormant breakpoints CAN still be toggled with a plain gutter click).
    if (ImGui::BeginPopup("ltg_code_gutter_ctx")) {
        const bool hasBreakpoint = (m_BreakpointLines.find(m_GutterContextLine) != m_BreakpointLines.end());
        ImGui::BeginDisabled(!m_BreakpointInteractionEnabled);
        if (!hasBreakpoint) {
            if (ImGui::MenuItem("Set Breakpoint")) {
                if (m_OnBreakpointToggled) {
                    m_OnBreakpointToggled(m_GutterContextLine, true);
                }
            }
        } else {
            if (ImGui::MenuItem("Remove Breakpoint")) {
                if (m_OnBreakpointToggled) {
                    m_OnBreakpointToggled(m_GutterContextLine, false);
                }
            }
        }
        ImGui::EndDisabled();
        ImGui::EndPopup();
    }
    // right-click on the text: "Watch <token>" (and future eval/expand actions)
    if (ImGui::BeginPopup("ltg_code_text_ctx")) {
        const std::string label = std::string("Watch \"") + m_ContextToken + "\"";
        if (ImGui::MenuItem(label.c_str())) {
            if (m_OnTokenContext) {
                m_OnTokenContext(m_ContextToken);
            }
        }
        ImGui::EndPopup();
    }
}

///////////////////////////////////////////////////////////////////////////////////
//// AUTOCOMPLETION (host-rendered popup over ImCode) /////////////////////////////
///////////////////////////////////////////////////////////////////////////////////

void CodeEditor::m_OnCharacterTyped(ImWchar aCharacter, int aLine, int aColumn) {
    // === completion popup: filter extension / dismissal =====================================
    if (m_CompletionPopupVisible) {
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
    if (aCharacter == '(' && !m_SignatureOpen && DebugSettings::ref()->isSignatureHelpEnabled()) {
        std::string target;
        std::string functionName;
        if (ltg::code::extractCallTargetAt(m_GetLineText(aLine), aColumn - 1, target, functionName)) {
            m_OpenSignature(target, functionName);
        }
    }
}

void CodeEditor::m_HandleCompletionKeys() {
    // pre-Render: the popup owns Up/Down/Enter/Escape while open. Runs when
    // m_CompletionPopupVisible is true at frame start; the same keys are then hidden from
    // ImCode::Render for this frame (see pushKeySuppression in OnImGui).
    const size_t count = m_CompletionFilteredEntries.size();
    if (count == 0) {
        m_OnCompletionCancelled();
        return;
    }
    // click outside the popup (previous frame's rect) dismisses — clicking a row is INSIDE
    // the rect and handled by the row's Selectable during the draw.
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        const ImVec2 mouse = ImGui::GetMousePos();
        const bool inPopup = (mouse.x >= m_CompletionPopupPos.x && mouse.x < m_CompletionPopupPos.x + m_CompletionPopupSize.x &&  //
                              mouse.y >= m_CompletionPopupPos.y && mouse.y < m_CompletionPopupPos.y + m_CompletionPopupSize.y);
        if (!inPopup) {
            m_OnCompletionCancelled();
            return;
        }
    }
    if (ImGui::IsKeyPressed(ImGuiKey_UpArrow)) {
        m_CompletionSelectedIndex = (m_CompletionSelectedIndex == 0) ? (count - 1) : (m_CompletionSelectedIndex - 1);
        m_CompletionSelectionChanged = true;
    }
    if (ImGui::IsKeyPressed(ImGuiKey_DownArrow)) {
        m_CompletionSelectedIndex = (m_CompletionSelectedIndex + 1) % count;
        m_CompletionSelectionChanged = true;
    }
    if (ImGui::IsKeyPressed(ImGuiKey_Enter, false) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter, false)) {
        m_OnCompletionAccepted(m_CompletionSelectedIndex);
        return;
    }
    if (ImGui::IsKeyPressed(ImGuiKey_Escape, false)) {
        m_OnCompletionCancelled();
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
    for (const size_t index : kept) {
        m_CompletionFilteredEntries.push_back(m_CompletionAllEntries[index]);
    }
    m_CompletionSelectedIndex = 0;
    m_CompletionSelectionChanged = true;
    if (m_CompletionFilteredEntries.empty()) {
        m_OnCompletionCancelled();  // empty list closes the popup silently
    } else {
        m_CompletionPopupVisible = true;
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
    m_CompletionPopupVisible = false;
    m_CompletionSelectedIndex = 0;
}

void CodeEditor::m_DrawCompletionPopup() {
    if (!m_CompletionPopupVisible || m_CompletionFilteredEntries.empty() || !m_Layout.valid) {
        return;
    }
    ImVec2 anchorCell;
    if (!m_TextPosToScreen(m_CompletionAnchorLine, m_CompletionAnchorColumn, anchorCell)) {
        return;
    }
    const float rowHeight = ImGui::GetTextLineHeightWithSpacing();
    const float visibleRows = (float)((m_CompletionFilteredEntries.size() < 10) ? m_CompletionFilteredEntries.size() : 10);
    const ImGuiStyle& style = ImGui::GetStyle();
    const ImVec2 popupSize(320.0f, visibleRows * rowHeight + style.WindowPadding.y * 2.0f);
    ImGui::SetNextWindowPos(ImVec2(anchorCell.x, anchorCell.y + m_Layout.lineHeight));
    ImGui::SetNextWindowSize(popupSize);
    // Tooltip layer so the popup sorts above the pane without ever taking the focus away
    // from the editor child (typing must keep flowing into ImCode while the popup shows).
    const ImGuiWindowFlags flags = ImGuiWindowFlags_Tooltip | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                                   ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;
    if (ImGui::Begin("##ltg_completion_popup", nullptr, flags)) {
        m_CompletionPopupPos = ImGui::GetWindowPos();
        m_CompletionPopupSize = ImGui::GetWindowSize();
        for (size_t i = 0; i < m_CompletionFilteredEntries.size(); ++i) {
            const auto& entry = m_CompletionFilteredEntries[i];
            const bool selected = (i == m_CompletionSelectedIndex);
            ImGui::PushID((int)i);
            if (ImGui::Selectable(entry.name.c_str(), selected)) {
                m_OnCompletionAccepted(i);
                ImGui::PopID();
                break;
            }
            if (selected && m_CompletionSelectionChanged) {
                ImGui::SetScrollHereY();
            }
            if (!entry.type.empty()) {
                const float typeWidth = ImGui::CalcTextSize(entry.type.c_str()).x;
                ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - typeWidth);
                ImGui::TextDisabled("%s", entry.type.c_str());
            }
            ImGui::PopID();
        }
        m_CompletionSelectionChanged = false;
    }
    ImGui::End();
}

///////////////////////////////////////////////////////////////////////////////////
//// SIGNATURE HELP (host-rendered tooltip over ImCode) ///////////////////////////
///////////////////////////////////////////////////////////////////////////////////

void CodeEditor::m_OpenSignature(const std::string& aTarget, const std::string& aFunctionName) {
    Ltg::SignatureInfo signature;
    ScriptingEngine::ref()->GetSignatureInfo(aTarget, aFunctionName, signature);
    if (signature.label.empty()) {
        return;  // unknown call — silently no tooltip
    }
    m_SignatureInfo = std::move(signature);
    m_SignatureTarget = aTarget;
    m_SignatureFunctionName = aFunctionName;
    m_SignatureArgIndex = 0;
    // anchor: line + BYTE col RIGHT AFTER the `(` (where the scan range starts each frame)
    const im::Code::Pos cursor = m_Editor.getCursor();
    m_SignatureAnchorLine = cursor.line;
    m_SignatureAnchorColumn = cursor.column;
    m_SignatureOpen = true;
}

void CodeEditor::m_CloseSignature() {
    m_SignatureOpen = false;
    m_SignatureInfo = {};
    m_SignatureTarget.clear();
    m_SignatureFunctionName.clear();
    m_SignatureAnchorLine = 0;
    m_SignatureAnchorColumn = 0;
    m_SignatureArgIndex = 0;
}

void CodeEditor::m_RecomputeSignatureState() {
    // recomputed every frame: re-scans the bytes between the opening `(` (captured at open
    // time) and the current caret to derive depth + comma count. handles Backspace, Delete,
    // paste, and arrow-key edits naturally — the source of truth is always the actual text.
    if (!m_SignatureOpen) {
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
    m_SignatureArgIndex = argIndex;
}

void CodeEditor::m_HandleSignatureDismissKeys() {
    // Escape is the only event that actively closes the tooltip — everything else (caret
    // moves, mouse clicks, Backspace, paste) is reconciled by m_RecomputeSignatureState().
    if (!m_SignatureOpen) {
        return;
    }
    if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
        m_CloseSignature();
    }
}

void CodeEditor::m_DrawSignatureTooltip() {
    if (!m_SignatureOpen || !m_Layout.valid) {
        return;
    }
    ImVec2 anchorCell;
    if (!m_TextPosToScreen(m_SignatureAnchorLine, m_SignatureAnchorColumn, anchorCell)) {
        return;
    }
    // bottom-left pivot: the tooltip sits just ABOVE the line being typed
    ImGui::SetNextWindowPos(ImVec2(anchorCell.x, anchorCell.y - 2.0f), ImGuiCond_Always, ImVec2(0.0f, 1.0f));
    const ImGuiWindowFlags flags = ImGuiWindowFlags_Tooltip | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                                   ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav |
                                   ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoInputs;
    if (ImGui::Begin("##ltg_signature_tooltip", nullptr, flags)) {
        ImGui::TextUnformatted(m_SignatureInfo.label.c_str());
        ImGui::SameLine(0.0f, 0.0f);
        ImGui::TextUnformatted("(");
        for (size_t i = 0; i < m_SignatureInfo.args.size(); ++i) {
            const auto& arg = m_SignatureInfo.args[i];
            ImGui::SameLine(0.0f, 0.0f);
            if ((int32_t)i == m_SignatureArgIndex) {
                ImGui::TextColored(ImVec4(1.0f, 0.78f, 0.0f, 1.0f), "%s: %s", arg.name.c_str(), arg.type.c_str());
            } else {
                ImGui::TextDisabled("%s: %s", arg.name.c_str(), arg.type.c_str());
            }
            if (i + 1 < m_SignatureInfo.args.size()) {
                ImGui::SameLine(0.0f, 0.0f);
                ImGui::TextUnformatted(", ");
            }
        }
        ImGui::SameLine(0.0f, 0.0f);
        ImGui::TextUnformatted(")");
    }
    ImGui::End();
}

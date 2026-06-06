#include "CodeEditor.h"
#include <ezlibs/ezTools.hpp>
#include <models/script/ScriptingEngine.h>

#include <filesystem>
#include <fstream>
#include <codecvt>

// imguipack's TextEditor was rewritten. several legacy entry points are gone (Mariana/RetroBlue
// palettes, IsShortTabsEnabled, ClearExtraCursors/ClearSelections, SetCursorPosition,
// SetErrorMarkers, GetLanguageDefinitionName, SelectRegion, GetCursorPosition...).
// the helpers below adapt the calls instead of removing the feature wholesale.

bool CodeEditor::init() {
    if (ImGui::GetIO().Fonts->Fonts.size() > 1U) {
        m_CodeFontPtr = ImGui::GetIO().Fonts->Fonts[1];
    }
    // right-click on the gutter toggles a breakpoint on that line (widget 0-based)
    m_Editor.SetLineNumberContextMenuCallback([this](int aLine) {
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
    // right-click on the text area → menu with "Watch <token>" when the click lands on an identifier.
    m_Editor.SetTextContextMenuCallback([this](int aLine, int aColumn) {
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
    // we ALWAYS fire (even with an empty token on whitespace/punctuation) so the consumer can
    // detect when the user has moved off a previously-hovered identifier.
    m_Editor.SetTextHoverCallback([this](int aLine, int aColumn) {
        if (m_OnHoverToken) {
            m_OnHoverToken(m_ExtractTokenAt(aLine, aColumn));
        }
    });
    // characters typed in the editor drive the autocompletion state machine (popup open/filter/close).
    m_Editor.SetCharacterTypedCallback([this](ImWchar aChar, int aLine, int aColumn) {
        m_OnCharacterTyped(aChar, aLine, aColumn);
    });
    // TextEditor owns the popup rendering + the Up/Down/Enter/Escape interception. it routes back
    // to the host via these two sinks when the user picks an entry or dismisses the popup.
    m_Editor.SetCompletionCallbacks(
        [this](size_t aSelectedIndex) { m_OnCompletionAccepted(aSelectedIndex); },
        [this]() { m_OnCompletionCancelled(); });
    // a narrow gutter decorator: a red dot marks a breakpoint, double-click toggles it
    m_Editor.SetLineDecorator(16.0f, [this](TextEditor::Decorator& aDecorator) {
        const int32_t line0 = aDecorator.line;  // zero-based
        ImGui::InvisibleButton("##bp", ImVec2(aDecorator.width, aDecorator.height));
        const bool hovered = ImGui::IsItemHovered();
        if (m_BreakpointInteractionEnabled && hovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
            const bool has = (m_BreakpointLines.find(line0) != m_BreakpointLines.end());
            if (m_OnBreakpointToggled) {
                m_OnBreakpointToggled(line0, !has);  // toggle
            }
        }
        const bool isBreakpoint = (m_BreakpointLines.find(line0) != m_BreakpointLines.end());
        const bool drawHoverHalo = hovered && m_BreakpointInteractionEnabled;  // no false affordance when interaction is off
        if (isBreakpoint || drawHoverHalo) {
            const ImVec2 rectMin = ImGui::GetItemRectMin();
            const float radius = (aDecorator.height - 6.0f) * 0.5f;
            const uint8_t bpAlpha = m_BreakpointInteractionEnabled ? 255 : 110;  // fade existing dot when toggling is off
            const ImU32 color = isBreakpoint ? IM_COL32(220, 40, 40, bpAlpha) : IM_COL32(220, 40, 40, 90);
            ImGui::GetWindowDrawList()->AddCircleFilled(ImVec2(rectMin.x + aDecorator.width * 0.5f, rectMin.y + aDecorator.height * 0.5f), radius, color);
        }
    });
    return true;
}

void CodeEditor::unit() {}

void CodeEditor::OnImGui() {
    bool isFocused = ImGui::IsWindowFocused();
    bool requestingGoToLinePopup = false;
    bool requestingFindPopup = false;
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("Edit")) {
            if (ImGui::MenuItem("Undo", "ALT-Backspace", nullptr, m_Editor.CanUndo())) {
                m_Editor.Undo();
            }
            if (ImGui::MenuItem("Redo", "Ctrl+Y", nullptr, m_Editor.CanRedo())) {
                m_Editor.Redo();
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Copy", "Ctrl+C", nullptr, m_Editor.AnyCursorHasSelection())) {
                m_Editor.Copy();
            }
            if (ImGui::MenuItem("Cut", "Ctrl+X", nullptr, m_Editor.AnyCursorHasSelection())) {
                m_Editor.Cut();
            }
            if (ImGui::MenuItem("Paste", "Ctrl+V", nullptr, ImGui::GetClipboardText() != nullptr)) {
                m_Editor.Paste();
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Select all", "Ctrl+A", nullptr)) {
                m_Editor.SelectAll();
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Find")) {
            if (ImGui::MenuItem("Go to line", "Ctrl+G")) {
                requestingGoToLinePopup = true;
            }
            if (ImGui::MenuItem("Find", "Ctrl+F")) {
                requestingFindPopup = true;
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Palette")) {
            // new TextEditor only ships Dark + Light palettes
            if (ImGui::MenuItem("Dark")) {
                m_Editor.SetPalette(TextEditor::GetDarkPalette());
            }
            if (ImGui::MenuItem("Light")) {
                m_Editor.SetPalette(TextEditor::GetLightPalette());
            }
            ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
    }

    if (m_CodeFontPtr) {
        ImGui::PushFont(m_CodeFontPtr);
        m_Editor.Render("TextEditor", ImVec2(), isFocused);
        ImGui::PopFont();
    } else {
        m_Editor.Render("TextEditor", ImVec2(), isFocused);
    }

    if (isFocused) {
        bool ctrlPressed = ImGui::GetIO().KeyCtrl;
        if (ctrlPressed) {
            if (ImGui::IsKeyDown(ImGuiKey_S)) {
                OnSaveCommand();
            }
            if (ImGui::IsKeyDown(ImGuiKey_R)) {
                OnReloadCommand();
            }
            if (ImGui::IsKeyDown(ImGuiKey_G)) {
                requestingGoToLinePopup = true;
            }
            if (ImGui::IsKeyDown(ImGuiKey_F)) {
                requestingFindPopup = true;
            }
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
            static int targetLineFixed;
            targetLineFixed = targetLine < 1 ? 0 : targetLine - 1;
            m_Editor.ClearCursors();
            m_Editor.SelectLine(targetLineFixed);
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
        const int32_t& toFindTextSize = (int32_t)strlen(m_CtrlfTextToFind);
        if ((ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter)) && toFindTextSize > 0) {
            m_Editor.ClearCursors();
            m_Editor.SelectNextOccurrenceOf(m_CtrlfTextToFind, toFindTextSize, m_CtrlfCaseSensitive);
        }
        if (ImGui::Button("Find all") && toFindTextSize > 0) {
            m_Editor.SelectAllOccurrencesOf(m_CtrlfTextToFind, toFindTextSize, m_CtrlfCaseSensitive);
        } else if (ImGui::IsKeyDown(ImGuiKey_Escape)) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void CodeEditor::SetSelection(int startLine, int startChar, int endLine, int endChar) {
    // new TextEditor does not expose SelectRegion; approximate with cursor placement
    (void)startLine;
    (void)startChar;
    m_Editor.SetCursor(endLine, endChar);
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
    m_Editor.SetLanguage(m_Type);
    m_Editor.SetText(vCode);
    m_UndoIndexInDisk = static_cast<int>(m_Editor.GetUndoIndex());  // freshly set text == on-disk state
}

std::string CodeEditor::GetCode() const {
    return m_Editor.GetText();
}

bool CodeEditor::IsModified() const {
    return m_Editor.GetUndoIndex() != static_cast<size_t>(m_UndoIndexInDisk);
}

void CodeEditor::MarkSaved() {
    m_UndoIndexInDisk = static_cast<int>(m_Editor.GetUndoIndex());
}

void CodeEditor::ClearErrorMarkers() {
    m_ErrorMarkers.clear();
    m_RebuildMarkers();
}

void CodeEditor::AddErrorMarker(const size_t& vErrorLine, const std::string& vErrorMsg) {
    m_ErrorMarkers[(int32_t)vErrorLine] = vErrorMsg;
    // AddMarker's textTooltip carries the message again, so the per-line error tooltip
    // is back (regression #2/#4 fixed); the cursor still jumps to the error line.
    m_RebuildMarkers();
    m_Editor.SetCursor((int32_t)vErrorLine, 0);
}

// Commands

void CodeEditor::OnReloadCommand() {
#if defined(__WIN32__) || defined(WIN32) || defined(_WIN32) || defined(__WIN64__) || defined(WIN64) || defined(_WIN64) || defined(_MSC_VER)
    std::ifstream t(ez::str::utf8Decode(m_RelatedFile).c_str());
#else
    std::ifstream t(m_RelatedFile);
#endif
    std::string str((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
    m_Editor.SetText(str);
    m_UndoIndexInDisk = 0;
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

std::string CodeEditor::m_ExtractTokenAt(int aLine, int aColumn) {
    // Extract the identifier at (aLine, aColumn). The TextEditor reports coordinates in VISUAL
    // columns (a tab counts as `tabSize - col % tabSize` cells), so we walk the raw line bytes
    // tracking visual-col to convert aColumn to a byte index before doing the identifier scan.
    if (aLine < 0 || aColumn < 0) {
        return std::string();
    }
    const std::string lineText = m_Editor.GetLineText(aLine);
    if (lineText.empty()) {
        return std::string();
    }
    const int32_t tabSize = m_Editor.GetTabSize();
    size_t byteIndex = 0;
    int32_t visualCol = 0;
    while (byteIndex < lineText.size() && visualCol < aColumn) {
        if (lineText[byteIndex] == '\t') {
            visualCol += tabSize - (visualCol % tabSize);
        } else {
            ++visualCol;
        }
        ++byteIndex;
    }
    // if aColumn landed inside a tab's expansion span, visualCol overshot — back up to the tab byte
    if (byteIndex > 0 && visualCol > aColumn) {
        --byteIndex;
    }
    if (byteIndex >= lineText.size()) {
        return std::string();
    }
    auto isIdentChar = [](char aChar) {
        return (aChar >= 'A' && aChar <= 'Z') || (aChar >= 'a' && aChar <= 'z') || (aChar >= '0' && aChar <= '9') || aChar == '_';
    };
    if (!isIdentChar(lineText[byteIndex])) {
        return std::string();
    }
    size_t leftBound = byteIndex;
    while (leftBound > 0 && isIdentChar(lineText[leftBound - 1])) {
        --leftBound;
    }
    size_t rightBound = byteIndex;
    while (rightBound < lineText.size() && isIdentChar(lineText[rightBound])) {
        ++rightBound;
    }
    if (rightBound <= leftBound) {
        return std::string();
    }
    // reject pure number literals (identifier rule: must not start with a digit)
    const char firstChar = lineText[leftBound];
    if (firstChar >= '0' && firstChar <= '9') {
        return std::string();
    }
    return lineText.substr(leftBound, rightBound - leftBound);
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
        if (m_CurrentExecLine >= 0) {
            m_Editor.SetCursor(m_CurrentExecLine, 0);  // scroll to the paused line
        }
    }
}

void CodeEditor::SetBreakpointInteractionEnabled(bool aEnabled) {
    m_BreakpointInteractionEnabled = aEnabled;
}

void CodeEditor::m_RebuildMarkers() {
    m_Editor.ClearMarkers();
    // breakpoints are drawn by the line decorator (a red dot); markers carry errors + current line
    for (const auto& errorMarker : m_ErrorMarkers) {
        m_Editor.AddMarker(errorMarker.first - 1, 0, IM_COL32(200, 0, 40, 80), "", errorMarker.second);
    }
    // current paused line: amber line number + translucent amber text
    if (m_CurrentExecLine >= 0) {
        m_Editor.AddMarker(m_CurrentExecLine, IM_COL32(255, 200, 0, 255), IM_COL32(255, 200, 0, 60), "current line", "");
    }
}

///////////////////////////////////////////////////////////////////////////////////
//// AUTOCOMPLETION (driven by TextEditor::SetCharacterTypedCallback) /////////////
///////////////////////////////////////////////////////////////////////////////////

bool CodeEditor::m_IsIdentChar(ImWchar aChar) {
    return (aChar >= 'A' && aChar <= 'Z') || (aChar >= 'a' && aChar <= 'z') ||
           (aChar >= '0' && aChar <= '9') || aChar == '_';
}

void CodeEditor::m_OnCharacterTyped(ImWchar aCharacter, int aLine, int aColumn) {
    // popup already open: any identifier char extends the filter; anything else dismisses.
    if (m_Editor.IsCompletionPopupOpen()) {
        if (m_IsIdentChar(aCharacter)) {
            m_CompletionFilter += static_cast<char>(aCharacter);
            m_RecomputeCompletionFiltered();  // pushes refreshed items to TextEditor (closes if empty)
        } else {
            m_Editor.CloseCompletionPopup();
            m_OnCompletionCancelled();
        }
        return;
    }

    // popup closed: open on `.` or `:` if the preceding word is a known catalog key.
    if (aCharacter != '.' && aCharacter != ':') {
        return;
    }
    // `aColumn` is the cursor RIGHT AFTER the trigger char. The identifier we want is to the LEFT of
    // the trigger, so we look one column further back (the trigger itself is at column - 1).
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
    m_RecomputeCompletionFiltered();      // builds the TextEditor::CompletionItem list and opens the popup
}

void CodeEditor::m_RecomputeCompletionFiltered() {
    m_CompletionFilteredEntries.clear();
    std::vector<TextEditor::CompletionItem> popupItems;
    for (const auto& entry : m_CompletionAllEntries) {
        // case-sensitive prefix match — fits Lua identifier conventions
        if (entry.name.size() >= m_CompletionFilter.size() &&
            entry.name.compare(0, m_CompletionFilter.size(), m_CompletionFilter) == 0) {
            m_CompletionFilteredEntries.push_back(entry);
            popupItems.push_back({entry.name, entry.type});
        }
    }
    // empty list closes the popup silently inside TextEditor — selection index drops to 0 there too
    m_Editor.OpenCompletionPopup(popupItems);
}

void CodeEditor::m_OnCompletionAccepted(size_t aSelectedIndex) {
    if (aSelectedIndex >= m_CompletionFilteredEntries.size()) {
        m_OnCompletionCancelled();
        return;
    }
    const auto& entry = m_CompletionFilteredEntries[aSelectedIndex];

    int currentLine = 0;
    int currentColumn = 0;
    m_Editor.GetCurrentCursor(currentLine, currentColumn);

    // select the filter region (from the anchor — right after the trigger — to the current cursor)
    // and replace it with the full entry name. SelectRegion + ReplaceTextInCurrentCursor does the
    // delete-then-insert as one transaction (cf. TextEditor::replaceTextInCurrentCursor).
    m_Editor.SelectRegion(m_CompletionAnchorLine, m_CompletionAnchorColumn, currentLine, currentColumn);
    m_Editor.ReplaceTextInCurrentCursor(entry.name);

    m_OnCompletionCancelled();  // share the state-reset path
}

void CodeEditor::m_OnCompletionCancelled() {
    m_CompletionTarget.clear();
    m_CompletionAllEntries.clear();
    m_CompletionFilteredEntries.clear();
    m_CompletionFilter.clear();
    m_CompletionAnchorLine = 0;
    m_CompletionAnchorColumn = 0;
}

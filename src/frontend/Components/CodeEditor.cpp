#include "CodeEditor.h"
#include <ezlibs/ezTools.hpp>
#include <models/script/ScriptingEngine.h>
#include <settings/DebugSettings.h>

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

    // signature-help: source of truth is the bytes between the opening `(` and the caret.
    // Recomputing every frame keeps depth + arg index in sync with Backspace, Delete, paste,
    // and arrow-key edits. Then Escape gets a chance to actively close the tooltip.
    m_RecomputeSignatureState();
    m_HandleSignatureDismissKeys();

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

size_t CodeEditor::GetUndoIndex() const {
    return m_Editor.GetUndoIndex();
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

void CodeEditor::SetPendingFontScale(float aScale) {
    m_Editor.SetPendingFontScale(aScale);
}

float CodeEditor::GetCurrentFontScale() const {
    return m_Editor.GetCurrentFontScale();
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
    // signature tooltip state is recomputed every frame in m_RecomputeSignatureState() — we
    // re-scan the bytes between the opening `(` and the current caret to derive depth + arg
    // index. that lets Backspace, Delete, paste, and arrow-key edits all stay in sync without
    // a per-event branch here.

    // === completion popup: filter extension / dismissal =====================================
    if (m_Editor.IsCompletionPopupOpen()) {
        if (m_IsIdentChar(aCharacter)) {
            m_CompletionFilter += static_cast<char>(aCharacter);
            m_RecomputeCompletionFiltered();  // pushes refreshed items to TextEditor (closes if empty)
            return;
        }
        m_Editor.CloseCompletionPopup();
        m_OnCompletionCancelled();
        // fall through — a non-ident char may also be a `(` that triggers the signature
    } else if ((aCharacter == '.' || aCharacter == ':') && DebugSettings::ref()->isAutoCompletionEnabled()) {
        // === completion trigger: `.` or `:` after a known catalog key ======================
        // `aColumn` is the cursor RIGHT AFTER the trigger char. The identifier we want is to
        // the LEFT of the trigger, so we look one column further back (the trigger itself
        // is at column - 1). gated by DebugSettings — when off, no popup ever opens.
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
        return;
    }

    // === signature trigger: `(` when no tooltip is currently open ==========================
    // we skip the trigger when the signature is already open: the first block above already
    // bumped the paren depth (nested call), and a nested signature tooltip would compete for
    // screen space with no clean UI to disambiguate. gated by DebugSettings.
    if (aCharacter == '(' && !m_Editor.IsSignatureTooltipOpen() && DebugSettings::ref()->isSignatureHelpEnabled()) {
        std::string target;
        std::string funcName;
        if (m_ExtractCallTargetAt(aLine, aColumn - 1, target, funcName)) {
            m_OpenSignature(target, funcName);
        }
    }
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

///////////////////////////////////////////////////////////////////////////////////
//// SIGNATURE HELP (host-side state, TextEditor owns the tooltip render) /////////
///////////////////////////////////////////////////////////////////////////////////

bool CodeEditor::m_ExtractCallTargetAt(int aLine, int aOpeningParenColumn, std::string& aoTarget, std::string& aoFunctionName) {
    // resolves `(target?, functionName)` from the bytes immediately to the LEFT of the `(`
    // at (aLine, aOpeningParenColumn). v1 accepts NO whitespace between the function name
    // and the `(`, and only a single `.` or `:` separator before an optional target.
    aoTarget.clear();
    aoFunctionName.clear();
    if (aLine < 0 || aOpeningParenColumn < 0) {
        return false;
    }
    const std::string lineText = m_Editor.GetLineText(aLine);
    if (lineText.empty()) {
        return false;
    }
    const int32_t tabSize = m_Editor.GetTabSize();

    // convert the visual column of the `(` to a byte index in lineText
    size_t parenByte = 0;
    int32_t visualCol = 0;
    while (parenByte < lineText.size() && visualCol < aOpeningParenColumn) {
        if (lineText[parenByte] == '\t') {
            visualCol += tabSize - (visualCol % tabSize);
        } else {
            ++visualCol;
        }
        ++parenByte;
    }
    if (parenByte > 0 && visualCol > aOpeningParenColumn) {
        --parenByte;
    }
    if (parenByte == 0 || parenByte > lineText.size()) {
        return false;  // nothing to the left of the `(`
    }

    auto isIdent = [](char aChar) {
        return (aChar >= 'A' && aChar <= 'Z') || (aChar >= 'a' && aChar <= 'z') || (aChar >= '0' && aChar <= '9') || aChar == '_';
    };

    // the function name ends RIGHT before the `(` — bail out on whitespace / punctuation
    if (!isIdent(lineText[parenByte - 1])) {
        return false;
    }
    size_t funcStart = parenByte - 1;
    while (funcStart > 0 && isIdent(lineText[funcStart - 1])) {
        --funcStart;
    }
    // reject pure number literals (identifier rule: must not start with a digit)
    if (lineText[funcStart] >= '0' && lineText[funcStart] <= '9') {
        return false;
    }
    aoFunctionName = lineText.substr(funcStart, parenByte - funcStart);

    // optional `target.` or `target:` separator right before the function name
    if (funcStart == 0) {
        return true;
    }
    const char sep = lineText[funcStart - 1];
    if (sep != ':' && sep != '.') {
        return true;
    }
    if (funcStart < 2 || !isIdent(lineText[funcStart - 2])) {
        return true;  // separator with nothing valid before — leave target empty
    }
    size_t targetEnd = funcStart - 1;  // exclusive boundary (the separator)
    size_t targetStart = targetEnd - 1;
    while (targetStart > 0 && isIdent(lineText[targetStart - 1])) {
        --targetStart;
    }
    if (lineText[targetStart] >= '0' && lineText[targetStart] <= '9') {
        return true;  // bad target — leave empty
    }
    aoTarget = lineText.substr(targetStart, targetEnd - targetStart);
    return true;
}

void CodeEditor::m_OpenSignature(const std::string& aTarget, const std::string& aFunctionName) {
    Ltg::SignatureInfo sig;
    ScriptingEngine::ref()->GetSignatureInfo(aTarget, aFunctionName, sig);
    if (sig.label.empty()) {
        return;  // unknown call — silently no tooltip
    }
    m_SignatureTarget = aTarget;
    m_SignatureFunctionName = aFunctionName;
    m_SignatureArgIndex = 0;
    // anchor: line + visual col RIGHT AFTER the `(` (where the scan range starts on the next frame).
    int currentLine = 0;
    int currentColumn = 0;
    m_Editor.GetCurrentCursor(currentLine, currentColumn);
    m_SignatureAnchorLine = currentLine;
    m_SignatureAnchorColumn = currentColumn;

    TextEditor::SignatureTooltip tooltip;
    tooltip.label = sig.label;
    tooltip.args.reserve(sig.args.size());
    for (const auto& arg : sig.args) {
        TextEditor::SignatureArg tArg;
        tArg.name = arg.name;
        tArg.type = arg.type;
        tooltip.args.push_back(std::move(tArg));
    }
    tooltip.currentArgIndex = 0;
    m_Editor.OpenSignatureTooltip(tooltip);
}

void CodeEditor::m_CloseSignature() {
    m_Editor.CloseSignatureTooltip();
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
    if (!m_Editor.IsSignatureTooltipOpen()) {
        return;
    }
    int currentLine = 0;
    int currentColumn = 0;
    m_Editor.GetCurrentCursor(currentLine, currentColumn);

    // close if the caret left the line of the opening `(`. v1 doesn't follow multi-line calls.
    if (currentLine != m_SignatureAnchorLine) {
        m_CloseSignature();
        return;
    }

    const std::string lineText = m_Editor.GetLineText(currentLine);
    const int32_t tabSize = m_Editor.GetTabSize();
    auto colToByte = [&](int32_t aCol) -> size_t {
        size_t byteIndex = 0;
        int32_t visualCol = 0;
        while (byteIndex < lineText.size() && visualCol < aCol) {
            if (lineText[byteIndex] == '\t') {
                visualCol += tabSize - (visualCol % tabSize);
            } else {
                ++visualCol;
            }
            ++byteIndex;
        }
        if (byteIndex > 0 && visualCol > aCol) {
            --byteIndex;
        }
        return byteIndex;
    };
    const size_t anchorByte = colToByte(m_SignatureAnchorColumn);
    const size_t cursorByte = colToByte(currentColumn);

    // close if the caret moved BEFORE the opening `(` (user backspaced past it or moved left).
    if (cursorByte < anchorByte) {
        m_CloseSignature();
        return;
    }

    // walk anchor → cursor, tracking paren depth (0 = our level) and comma count at depth 0.
    // string literals are skipped so a `,` or `(` inside `"hello, world"` doesn't disturb us.
    int32_t depth = 0;
    int32_t commaCount = 0;
    bool inString = false;
    char stringDelim = 0;
    for (size_t i = anchorByte; i < cursorByte && i < lineText.size(); ++i) {
        const char c = lineText[i];
        if (inString) {
            if (c == '\\' && i + 1 < lineText.size()) {
                ++i;  // skip escaped char
                continue;
            }
            if (c == stringDelim) {
                inString = false;
            }
            continue;
        }
        if (c == '"' || c == '\'') {
            inString = true;
            stringDelim = c;
            continue;
        }
        if (c == '(') {
            ++depth;
        } else if (c == ')') {
            --depth;
            if (depth < 0) {
                // caret moved past the matching `)` — call is finished
                m_CloseSignature();
                return;
            }
        } else if (c == ',' && depth == 0) {
            ++commaCount;
        }
    }

    if (commaCount != m_SignatureArgIndex) {
        m_SignatureArgIndex = commaCount;
        m_Editor.UpdateSignatureCurrentArg(m_SignatureArgIndex);
    }
}

void CodeEditor::m_HandleSignatureDismissKeys() {
    // Escape is the only event that actively closes the tooltip — everything else (caret moves,
    // mouse clicks, Backspace, paste) is reconciled by m_RecomputeSignatureState() each frame.
    if (!m_Editor.IsSignatureTooltipOpen()) {
        return;
    }
    if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
        m_CloseSignature();
    }
}

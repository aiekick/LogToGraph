#include "CodeEditor.h"
#include <ezlibs/ezTools.hpp>

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
            ImGui::GetWindowDrawList()->AddCircleFilled(
                ImVec2(rectMin.x + aDecorator.width * 0.5f, rectMin.y + aDecorator.height * 0.5f), radius, color);
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
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Save", "Ctrl+S")) {
                OnSaveCommand();
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit")) {
            bool ro = m_Editor.IsReadOnlyEnabled();
            if (ImGui::MenuItem("Read only mode enabled", nullptr, &ro)) {
                m_Editor.SetReadOnlyEnabled(ro);
            }
            bool ai = m_Editor.IsAutoIndentEnabled();
            if (ImGui::MenuItem("Auto indent on enter enabled", nullptr, &ai)) {
                m_Editor.SetAutoIndentEnabled(ai);
            }
            ImGui::Separator();

            if (ImGui::MenuItem("Undo", "ALT-Backspace", nullptr, !ro && m_Editor.CanUndo())) {
                m_Editor.Undo();
            }
            if (ImGui::MenuItem("Redo", "Ctrl+Y", nullptr, !ro && m_Editor.CanRedo())) {
                m_Editor.Redo();
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Copy", "Ctrl+C", nullptr, m_Editor.AnyCursorHasSelection())) {
                m_Editor.Copy();
            }
            if (ImGui::MenuItem("Cut", "Ctrl+X", nullptr, !ro && m_Editor.AnyCursorHasSelection())) {
                m_Editor.Cut();
            }
            if (ImGui::MenuItem("Paste", "Ctrl+V", nullptr, !ro && ImGui::GetClipboardText() != nullptr)) {
                m_Editor.Paste();
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Select all", "Ctrl+A", nullptr)) {
                m_Editor.SelectAll();
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View")) {
            ImGui::SliderInt("Tab size", &m_TabSize, 1, 8);
            ImGui::SliderFloat("Line spacing", &m_LineSpacing, 1.0f, 2.0f);
            m_Editor.SetTabSize(m_TabSize);
            m_Editor.SetLineSpacing(m_LineSpacing);
            static bool showSpaces = m_Editor.IsShowWhitespacesEnabled();
            if (ImGui::MenuItem("Show spaces", nullptr, &showSpaces)) {
                m_Editor.SetShowWhitespacesEnabled(!(m_Editor.IsShowWhitespacesEnabled()));
            }
            static bool showLineNumbers = m_Editor.IsShowLineNumbersEnabled();
            if (ImGui::MenuItem("Show line numbers", nullptr, &showLineNumbers)) {
                m_Editor.SetShowLineNumbersEnabled(!(m_Editor.IsShowLineNumbersEnabled()));
            }
            // short tabs option dropped from new TextEditor
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

        const auto* lang = m_Editor.GetLanguage();
        ImGui::Text("%6d lines | %s | %s",
                    m_Editor.GetLineCount(),
                    m_Editor.IsOverwriteEnabled() ? "Ovr" : "Ins",
                    lang ? "lang" : "plain");

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
        m_Editor.AddMarker(errorMarker.first, 0, IM_COL32(200, 0, 40, 80), "", errorMarker.second);
    }
    // current paused line: amber line number + translucent amber text
    if (m_CurrentExecLine >= 0) {
        m_Editor.AddMarker(m_CurrentExecLine, IM_COL32(255, 200, 0, 255), IM_COL32(255, 200, 0, 60), "current line", "");
    }
}

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
}

void CodeEditor::ClearErrorMarkers() {
    m_ErrorMarkers.clear();
    // new TextEditor exposes ClearMarkers() instead of SetErrorMarkers(map)
    m_Editor.ClearMarkers();
}

void CodeEditor::AddErrorMarker(const size_t& vErrorLine, const std::string& vErrorMsg) {
    m_ErrorMarkers[(int32_t)vErrorLine] = vErrorMsg;
    // new TextEditor: jump to the line via cursor; rich marker rendering is no longer
    // wired up through SetErrorMarkers(map). this keeps the UX (jump-to-error) intact.
    (void)vErrorMsg;
    m_Editor.SelectLine((int32_t)vErrorLine);
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

void CodeEditor::OnSaveCommand() {}

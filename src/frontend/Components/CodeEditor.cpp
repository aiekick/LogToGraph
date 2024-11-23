#include "CodeEditor.h"
#include <ezlibs/ezTools.hpp>

#include <filesystem>
#include <fstream>
#include <codecvt>

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
            /*if (!m_RelatedFile.empty() && ImGui::MenuItem("Reload", "Ctrl+R")) {
                OnReloadCommand();
            }*/
            /*if (ImGui::MenuItem("Load from")) {
                OnLoadFromCommand();
            }*/
            if (ImGui::MenuItem("Save", "Ctrl+S")) {
                OnSaveCommand();
            }
            /*if (this->hasAssociatedFile && ImGui::MenuItem("Show in file explorer"))
                Utils::ShowInFileExplorer(this->m_RelatedFile);
            if (this->hasAssociatedFile && this->onShowInFolderViewCallback != nullptr && this->createdFromFolderView > -1 &&
                ImGui::MenuItem("Show in folder view")){
                this->onShowInFolderViewCallback(this->m_RelatedFile, this->createdFromFolderView);
            }
            */
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
            static bool showShortTabs = m_Editor.IsShortTabsEnabled();
            if (ImGui::MenuItem("Short tabs", nullptr, &showShortTabs)) {
                m_Editor.SetShortTabsEnabled(!(m_Editor.IsShortTabsEnabled()));
            }
            /*if (ImGui::BeginMenu("Language"))
            {
                for (int i = (int)TextEditor::LanguageDefinitionId::None; i <= (int)TextEditor::LanguageDefinitionId::Hlsl; i++)
                {
                    bool isSelected = i == (int)m_Editor.GetLanguageDefinition();
                    if (ImGui::MenuItem(languageDefinitionToName[(TextEditor::LanguageDefinitionId)i], nullptr, &isSelected))
                        m_Editor.SetLanguageDefinition((TextEditor::LanguageDefinitionId)i);
                }
                ImGui::EndMenu();
            }*/
            /*if (ImGui::BeginMenu("Color scheme"))
            {
                for (int i = (int)TextEditor::PaletteId::Dark; i <= (int)TextEditor::PaletteId::RetroBlue; i++)
                {
                    bool isSelected = i == (int)m_Editor.GetPalette();
                    if (ImGui::MenuItem(colorPaletteToName[(TextEditor::PaletteId)i], nullptr, &isSelected))
                        m_Editor.SetPalette((TextEditor::PaletteId)i);
                }
                ImGui::EndMenu();
            }*/
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
            if (ImGui::MenuItem("Mariana")) {
                m_Editor.SetPalette(TextEditor::GetMarianaPalette());
            }
            if (ImGui::MenuItem("Dark")) {
                m_Editor.SetPalette(TextEditor::GetDarkPalette());
            }
            if (ImGui::MenuItem("Light")) {
                m_Editor.SetPalette(TextEditor::GetLightPalette());
            }
            if (ImGui::MenuItem("RetroBlue")) {
                m_Editor.SetPalette(TextEditor::GetRetroBluePalette());
            }
            ImGui::EndMenu();
        }

        int line, column;
        m_Editor.GetCursorPosition(line, column);
        ImGui::Text("%6d/%-6d %6d lines | %s | %s",
                    line + 1,
                    column + 1,
                    m_Editor.GetLineCount(),
                    m_Editor.IsOverwriteEnabled() ? "Ovr" : "Ins",
                    m_Editor.GetLanguageDefinitionName());

        ImGui::EndMenuBar();
    }

    if (m_CodeFontPtr) {
        ImGui::PushFont(m_CodeFontPtr);
        isFocused |= m_Editor.Render("TextEditor", isFocused);
        ImGui::PopFont();
    } else {
        isFocused |= m_Editor.Render("TextEditor", isFocused);
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
            m_Editor.ClearExtraCursors();
            m_Editor.ClearSelections();
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
        if (requestingFindPopup)
            ImGui::SetKeyboardFocusHere();
        ImGui::InputText("To find", m_CtrlfTextToFind, FIND_POPUP_TEXT_FIELD_LENGTH, ImGuiInputTextFlags_AutoSelectAll);
        const int32_t& toFindTextSize = (int32_t)strlen(m_CtrlfTextToFind);
        if ((ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter)) && toFindTextSize > 0) {
            m_Editor.ClearExtraCursors();
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
    EZ_TOOLS_DEBUG_BREAK;
    m_Editor.SetCursorPosition(endLine, endChar);
    m_Editor.SelectRegion(startLine, startChar, endLine, endChar);
}

void CodeEditor::SetRelatedFile(const std::string& vFile) {
    m_RelatedFile = vFile;
}

const std::string& CodeEditor::GetRelatedFile() {
    return m_RelatedFile;
}

void CodeEditor::OnFolderViewDeleted(int folderViewId) {
    EZ_TOOLS_DEBUG_BREAK;
    if (m_CreatedFromFolderView == folderViewId) {
        m_CreatedFromFolderView = -1;
    }
}

void CodeEditor::SetShowDebugPanel(bool value) {
    EZ_TOOLS_DEBUG_BREAK;
    m_ShowDebugPanel = value;
}

void CodeEditor::SetCode(const std::string& vCode, const TextEditor::LanguageDefinition& vType) {
    m_Type = vType;
    m_Editor.SetLanguageDefinition(m_Type);
    m_Editor.SetText(vCode);
    // m_Editor.Render("CodeEditor");
}

void CodeEditor::ClearErrorMarkers() {
    m_ErrorMarkers.clear();
}

void CodeEditor::AddErrorMarker(const size_t& vErrorLine, const std::string& vErrorMsg) {
    m_ErrorMarkers[(int32_t)vErrorLine] = vErrorMsg;
    m_Editor.SetErrorMarkers(m_ErrorMarkers);
    m_Editor.SelectLine((int32_t)vErrorLine);
    m_Editor.SetCursorPosition((int32_t)vErrorLine, 0);
}

// Commands

void CodeEditor::OnReloadCommand() {
    EZ_TOOLS_DEBUG_BREAK;
#if defined(__WIN32__) || defined(WIN32) || defined(_WIN32) || defined(__WIN64__) || defined(WIN64) || defined(_WIN64) || defined(_MSC_VER)
    std::ifstream t(ez::str::utf8Decode(m_RelatedFile).c_str());
#else
    std::ifstream t(m_RelatedFile);
#endif
    std::string str((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
    m_Editor.SetText(str);
    m_UndoIndexInDisk = 0;
}

void CodeEditor::OnLoadFromCommand() {
    EZ_TOOLS_DEBUG_BREAK;
    /*std::vector<std::string> selection = pfd::open_file("Open file", "", { "Any file", "*" }).result();
    if (selection.size() == 0) {
        std::cout << "File not loaded\n";
    } else {
        std::ifstream t(Utf8ToWstring(selection[0]));
        std::string str((std::istreambuf_iterator<char>(t)),
        std::istreambuf_iterator<char>());
        m_Editor.SetText(str);
        auto pathObject = std::filesystem::path(selection[0]);
        auto lang = extensionToLanguageDefinition.find(pathObject.extension().string());
        if (lang != extensionToLanguageDefinition.end())
            m_Editor.SetLanguageDefinition(extensionToLanguageDefinition[pathObject.extension().string()]);
    }
    undoIndexInDisk = -1; // assume they are loading text from some other file*/
}

void CodeEditor::OnSaveCommand() {
    EZ_TOOLS_DEBUG_BREAK;
    /*std::string textToSave = m_Editor.GetText();
    std::string destination = hasAssociatedFile ?
        m_RelatedFile :
        pfd::save_file("Save file", "", { "Any file", "*" }).result();
    if (destination.length() > 0) {
        m_RelatedFile = destination;
        hasAssociatedFile = true;
        panelName = std::filesystem::path(destination).filename().string() + "##" + std::to_string((int)this);
        std::ofstream outFile(Utils::Utf8ToWstring(destination), std::ios::binary);
        outFile << textToSave;
        outFile.close();
    }
    undoIndexInDisk = m_Editor.GetUndoIndex();*/
}

#pragma once

#include <imguipack.h>

#include <cstdint>
#include <map>
#include <string>
#include <functional>
#include <unordered_set>

#define FIND_POPUP_TEXT_FIELD_LENGTH 128

// new imguipack's TextEditor exposes `const TextEditor::Language*` instead of `LanguageDefinition`
using CodeEditorLanguage = const TextEditor::Language*;

class CodeEditor {
public:
    typedef void (*OnFocusedCallback)(int folderViewId);
    typedef void (*OnShowInFolderViewCallback)(const std::string& filePath, int folderViewId);

private:
    OnFocusedCallback onFocusedCallback = nullptr;
    OnShowInFolderViewCallback onShowInFolderViewCallback = nullptr;
    CodeEditorLanguage m_Type = nullptr;
    std::map<int32_t, std::string> m_ErrorMarkers;
    std::unordered_set<int32_t> m_BreakpointLines;  // widget 0-based lines
    int32_t m_CurrentExecLine = -1;                 // widget 0-based, -1 = none
    int64_t m_LastBreakpointsRevision = -1;         // skip SetBreakpoints work when revision matches
    bool m_BreakpointInteractionEnabled = true;     // when false, gutter cannot set/remove breakpoints
    std::function<void(int32_t aLine, bool aAdd)> m_OnBreakpointToggled;
    std::function<void(const std::string& aToken)> m_OnTokenContext;  // right-click → Watch (and future eval/expand actions)
    std::function<void()> m_OnSave;
    ImFont* m_CodeFontPtr = nullptr;
    int m_Id = -1;
    int m_CreatedFromFolderView = -1;
    TextEditor m_Editor;
    bool m_ShowDebugPanel = false;
    std::string m_PanelName;
    std::string m_RelatedFile;
    int m_TabSize = 4;
    float m_LineSpacing = 1.0f;
    int m_UndoIndexInDisk = 0;
    char m_CtrlfTextToFind[FIND_POPUP_TEXT_FIELD_LENGTH] = "";
    bool m_CtrlfCaseSensitive = false;

public:
    CodeEditor() = default;
    ~CodeEditor() = default;
    bool init();
    void unit();

    void OnImGui();
    void SetSelection(int startLine, int startChar, int endLine, int endChar);
    void SetRelatedFile(const std::string& vFile);
    const std::string& GetRelatedFile();
    void OnFolderViewDeleted(int folderViewId);
    void SetShowDebugPanel(bool value);

    void SetCode(const std::string& vCode, CodeEditorLanguage vType);
    std::string GetCode() const;
    bool IsModified() const;  // true if edited since the last SetCode / MarkSaved
    void MarkSaved();

    void ClearErrorMarkers();
    void AddErrorMarker(const size_t& vErrorLine, const std::string& vErrorMsg);

    // debugger integration — lines are the widget's 0-based numbers
    void SetBreakpointToggledCallback(std::function<void(int32_t aLine, bool aAdd)> aCallback);
    void SetSaveCallback(std::function<void()> aCallback);
    // right-click on the text → menu with "Watch <token>"; the callback receives the extracted identifier
    void SetTokenContextCallback(std::function<void(const std::string& aToken)> aCallback);
    void SetBreakpoints(const std::unordered_set<int32_t>& aZeroBasedLines, int64_t aRevision);
    void SetCurrentExecLine(int32_t aZeroBasedLine);
    void SetBreakpointInteractionEnabled(bool aEnabled);

private:
    void OnReloadCommand();
    void OnLoadFromCommand();
    void OnSaveCommand();
    void m_RebuildMarkers();
    std::string m_ExtractTokenAt(int aLine, int aColumn);  // identifier-only; returns "" if click is on punctuation/whitespace
};

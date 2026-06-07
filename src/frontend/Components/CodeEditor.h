#pragma once

#include <imguipack.h>
#include <apis/LtgPluginApi.h>  // brings Ltg::CompletionEntry + Ltg::SignatureInfo

#include <cstdint>
#include <map>
#include <string>
#include <functional>
#include <unordered_set>
#include <vector>

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
    std::function<void(const std::string& aToken)> m_OnHoverToken;   // mouse hover over text → token under cursor (empty if punctuation)
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

    // autocompletion state — the popup itself (rendering, Up/Down/Enter/Escape keys, click-outside)
    // lives inside TextEditor now. the host only owns the catalog snapshot + the filter accumulator
    // + the anchor of the trigger char (so the on-accept callback knows what to replace).
    std::string m_CompletionTarget;                       // "ltg", "math", ... — catalog key
    std::vector<Ltg::CompletionEntry> m_CompletionAllEntries;  // catalog snapshot for the target
    std::vector<Ltg::CompletionEntry> m_CompletionFilteredEntries;  // last filtered subset — indexes match the items pushed to TextEditor
    std::string m_CompletionFilter;
    int32_t m_CompletionAnchorLine = 0;
    int32_t m_CompletionAnchorColumn = 0;

    // signature-help state — the tooltip itself (rendering, anchor capture) lives inside TextEditor.
    // the host owns the byte-scan reconciliation that derives the current arg index from the
    // text between the opening `(` and the caret. closed by host on Escape, on the caret moving
    // before the anchor, on a different line, or on the matching `)` being passed.
    std::string m_SignatureTarget;
    std::string m_SignatureFunctionName;
    int32_t m_SignatureAnchorLine = 0;     // line of the `(` that opened the tooltip
    int32_t m_SignatureAnchorColumn = 0;   // visual col RIGHT AFTER the `(` — where the scan starts
    int32_t m_SignatureArgIndex = 0;       // cached, mirrors the last value pushed to TextEditor

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
    // mouse hover over text → token under the mouse (empty when on whitespace/punctuation). fires every
    // frame the mouse is over the text area; consumer is expected to do its own per-frame tracking.
    void SetHoverTokenCallback(std::function<void(const std::string& aToken)> aCallback);
    void SetBreakpoints(const std::unordered_set<int32_t>& aZeroBasedLines, int64_t aRevision);
    void SetCurrentExecLine(int32_t aZeroBasedLine);
    void SetBreakpointInteractionEnabled(bool aEnabled);

private:
    void OnReloadCommand();
    void OnLoadFromCommand();
    void OnSaveCommand();
    void m_RebuildMarkers();
    std::string m_ExtractTokenAt(int aLine, int aColumn);  // identifier-only; returns "" if click is on punctuation/whitespace

    // autocompletion plumbing
    void m_OnCharacterTyped(ImWchar aCharacter, int aLine, int aColumn);
    void m_RecomputeCompletionFiltered();                  // refilter m_CompletionAllEntries by m_CompletionFilter and push to TextEditor
    void m_OnCompletionAccepted(size_t aSelectedIndex);    // TextEditor → host: user picked an entry
    void m_OnCompletionCancelled();                        // TextEditor → host: Escape or click outside
    static bool m_IsIdentChar(ImWchar aChar);

    // signature-help plumbing
    bool m_ExtractCallTargetAt(int aLine, int aOpeningParenColumn, std::string& aoTarget, std::string& aoFunctionName);
    void m_OpenSignature(const std::string& aTarget, const std::string& aFunctionName);
    void m_CloseSignature();
    void m_RecomputeSignatureState();     // called every frame from OnImGui — re-scans the line to derive depth + arg index
    void m_HandleSignatureDismissKeys();  // Escape only — caret-movement / click / Backspace are handled by m_RecomputeSignatureState
};

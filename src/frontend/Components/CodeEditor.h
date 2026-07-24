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

// imguipack's editor is im::Code (ImCode). Languages are plain names resolved by
// ImCode's lexer registry: "cpp" / "c" / "glsl" / "sql" / "lua" (anything else = plain text).
using CodeEditorLanguage = const char*;

class CodeEditor {
public:
    typedef void (*OnFocusedCallback)(int folderViewId);
    typedef void (*OnShowInFolderViewCallback)(const std::string& filePath, int folderViewId);

private:
    CodeEditorLanguage m_Type = nullptr;
    std::map<int32_t, std::string> m_ErrorMarkers;
    std::unordered_set<int32_t> m_BreakpointLines;  // widget 0-based lines
    int32_t m_CurrentExecLine = -1;                 // widget 0-based, -1 = none
    int64_t m_LastBreakpointsRevision = -1;         // skip SetBreakpoints work when revision matches
    bool m_BreakpointInteractionEnabled = true;     // when false, gutter breakpoints are dormant (visual fade)
    std::function<void(int32_t aLine, bool aAdd)> m_OnBreakpointToggled;
    std::function<void(const std::string& aToken)> m_OnTokenContext;  // right-click → Watch (and future eval/expand actions)
    std::function<void(const std::string& aToken)> m_OnHoverToken;   // mouse hover over text → token under cursor (empty if punctuation)
    std::function<void()> m_OnSave;
    ImFont* m_CodeFontPtr = nullptr;
    int m_CreatedFromFolderView = -1;
    im::Code m_Editor;
    bool m_ShowDebugPanel = false;
    std::string m_PanelName;
    std::string m_RelatedFile;
    char m_CtrlfTextToFind[FIND_POPUP_TEXT_FIELD_LENGTH] = "";
    bool m_CtrlfCaseSensitive = false;

    // --- text mirror -------------------------------------------------------------------
    // ImCode has no undo-index/dirty readback, so the host derives both from the text
    // itself: a per-frame hash refresh bumps m_Revision on any content change, and
    // IsModified compares against the hash captured at SetCode/MarkSaved time. The line
    // cache backs every (line, byte-column) computation (token extraction).
    std::vector<std::string> m_CachedLines;
    size_t m_TextHash = 0;
    size_t m_SavedTextHash = 0;
    size_t m_Revision = 0;

    // per-editor persistent font scale — backed by ImCode's local zoom (Ctrl+MouseWheel),
    // which is readable/settable since v0.2 so the host can persist/restore it.
    float m_PendingFontScale = 0.0f;  // one-shot, applied on the next OnImGui (0 = none)

    // autocompletion state — ImCode renders the popup and owns the keys while it is open;
    // the host owns the catalog snapshot + the filter accumulator + the anchor of the
    // trigger char (so the on-accept callback knows what to replace).
    std::string m_CompletionTarget;                            // "ltg", "math", ... — catalog key
    std::vector<Ltg::CompletionEntry> m_CompletionAllEntries;  // catalog snapshot for the target
    std::vector<Ltg::CompletionEntry> m_CompletionFilteredEntries;  // last filtered subset — indexes match the popup rows
    std::string m_CompletionFilter;
    int32_t m_CompletionAnchorLine = 0;
    int32_t m_CompletionAnchorColumn = 0;  // BYTE column right after the trigger char

    // signature-help state — ImCode renders the tooltip; the host owns the byte-scan
    // reconciliation that derives the current arg index from the text between the opening
    // `(` and the caret. closed on Escape, on the caret moving before the anchor, on a
    // different line, or on the matching `)` being passed.
    int32_t m_SignatureAnchorLine = 0;    // line of the `(` that opened the tooltip
    int32_t m_SignatureAnchorColumn = 0;  // BYTE col RIGHT AFTER the `(` — where the scan starts
    int32_t m_SignatureArgIndex = 0;

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
    size_t GetUndoIndex() const;  // monotonic edit counter — used to drive completion refresh on edit

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
    void MoveCursorTo(int32_t aZeroBasedLine, int32_t aZeroBasedColumn);  // jump caret; caller decides when to sync

    // per-editor persistent font scale — applied on the next OnImGui. GetCurrentFontScale
    // reads back the effective ImCode zoom; the host polls it after OnImGui to detect
    // interactive Ctrl+MouseWheel zoom and persist.
    void SetPendingFontScale(float aScale);
    float GetCurrentFontScale() const;
    void SetBreakpointInteractionEnabled(bool aEnabled);

private:
    void OnReloadCommand();
    void OnLoadFromCommand();
    void OnSaveCommand();
    void m_RebuildMarkers();
    void m_ApplyPalette(bool aDark);

    // text mirror
    void m_RefreshTextCache();  // re-hash + re-split when the content changed; bumps m_Revision
    const std::string& m_GetLineText(int32_t aLine) const;  // "" when out of range
    std::string m_ExtractTokenAt(int aLine, int aByteColumn);

    // autocompletion plumbing (state machine — ImCode owns the popup UI)
    void m_OnCharacterTyped(unsigned int aCharacter, int aLine, int aColumn);
    void m_RecomputeCompletionFiltered();
    void m_OnCompletionAccepted(size_t aSelectedIndex);
    void m_OnCompletionCancelled();

    // signature-help plumbing (state machine — ImCode owns the tooltip UI)
    void m_OpenSignature(const std::string& aTarget, const std::string& aFunctionName);
    void m_CloseSignature();
    void m_RecomputeSignatureState();     // called every frame — re-scans the line to derive depth + arg index
    void m_HandleSignatureDismissKeys();  // Escape only — everything else is handled by m_RecomputeSignatureState
};

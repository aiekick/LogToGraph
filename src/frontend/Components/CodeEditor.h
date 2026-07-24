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

// imguipack's editor is now im::Code (ImCode). Languages are plain names resolved by
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
    // ImCode has no undo-index/dirty readback in v0.1, so the host derives both from the
    // text itself: a per-frame hash refresh bumps m_Revision on any content change, and
    // IsModified compares against the hash captured at SetCode/MarkSaved time. The line
    // cache backs every (line, byte-column) computation (token extraction, hit-testing).
    std::vector<std::string> m_CachedLines;
    size_t m_TextHash = 0;
    size_t m_SavedTextHash = 0;
    size_t m_Revision = 0;

    // --- render-layout mirror ----------------------------------------------------------
    // ImCode draws inside a child window and exposes no screen<->text mapping. The host
    // reproduces its layout math (same formulas as ImCode::Render) from the child window's
    // Pos/Scroll — looked up via imgui_internal — to place the completion popup / signature
    // tooltip at the caret and to hit-test mouse hover / right-clicks.
    struct EditorLayout {
        bool valid = false;
        bool focused = false;  // the editor child window owns nav focus
        ImVec2 childPos{};
        ImVec2 childSize{};
        ImVec2 scroll{};
        float charWidth = 0.0f;
        float lineHeight = 0.0f;
        float fontSize = 0.0f;
        float gutterWidth = 0.0f;
    };
    EditorLayout m_Layout;

    // per-editor persistent font scale (Ctrl+MouseWheel). The host owns the zoom: the wheel
    // event is eaten before ImCode::Render so its internal (non-persistable) zoom never
    // engages, and the scale is applied through PushFont(font, base * scale).
    float m_FontScale = 1.0f;
    float m_PendingFontScale = 0.0f;  // one-shot, applied on the next OnImGui (0 = none)

    // autocompletion state — the popup is rendered by the host (ImCode has no popup UI).
    // the host owns the catalog snapshot + the filter accumulator + the anchor of the
    // trigger char (so the on-accept callback knows what to replace).
    std::string m_CompletionTarget;                            // "ltg", "math", ... — catalog key
    std::vector<Ltg::CompletionEntry> m_CompletionAllEntries;  // catalog snapshot for the target
    std::vector<Ltg::CompletionEntry> m_CompletionFilteredEntries;  // last filtered subset — indexes match the popup rows
    std::string m_CompletionFilter;
    int32_t m_CompletionAnchorLine = 0;
    int32_t m_CompletionAnchorColumn = 0;  // BYTE column right after the trigger char
    bool m_CompletionPopupVisible = false;
    size_t m_CompletionSelectedIndex = 0;
    bool m_CompletionSelectionChanged = false;  // scroll the popup to the selected row
    ImVec2 m_CompletionPopupPos{};
    ImVec2 m_CompletionPopupSize{};  // last drawn rect, for the click-outside test

    // signature-help state — the tooltip is rendered by the host. the byte-scan
    // reconciliation derives the current arg index from the text between the opening `(`
    // and the caret. closed on Escape, on the caret moving before the anchor, on a
    // different line, or on the matching `)` being passed.
    bool m_SignatureOpen = false;
    Ltg::SignatureInfo m_SignatureInfo;
    std::string m_SignatureTarget;
    std::string m_SignatureFunctionName;
    int32_t m_SignatureAnchorLine = 0;    // line of the `(` that opened the tooltip
    int32_t m_SignatureAnchorColumn = 0;  // BYTE col RIGHT AFTER the `(` — where the scan starts
    int32_t m_SignatureArgIndex = 0;

    // gutter / text right-click context menus (host popups over ImCode's canvas)
    int32_t m_GutterContextLine = -1;
    std::string m_ContextToken;

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
    // reads back the effective scale; the host polls it after OnImGui to detect interactive
    // Ctrl+MouseWheel zoom and persist.
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

    // layout mirror + coordinate mapping (ImCode's formulas)
    void m_CaptureLayout(const char* aChildStrId, ImGuiID aChildId, const char* aParentName, float aFontSize, float aLineHeight, float aCharWidth);
    bool m_ScreenToTextPos(const ImVec2& aScreen, int32_t& aoLine, int32_t& aoByteColumn) const;
    bool m_TextPosToScreen(int32_t aLine, int32_t aByteColumn, ImVec2& aoScreen) const;  // top-left of the cell
    int32_t m_DisplayColumn(int32_t aLine, int32_t aByteColumn) const;
    int32_t m_ByteColumnFromDisplay(int32_t aLine, float aDisplayColumns) const;

    // autocompletion plumbing
    void m_OnCharacterTyped(ImWchar aCharacter, int aLine, int aColumn);
    void m_HandleCompletionKeys();  // pre-Render: Up/Down/Enter/Escape + click-outside
    void m_RecomputeCompletionFiltered();
    void m_OnCompletionAccepted(size_t aSelectedIndex);
    void m_OnCompletionCancelled();
    void m_DrawCompletionPopup();

    // signature-help plumbing
    void m_OpenSignature(const std::string& aTarget, const std::string& aFunctionName);
    void m_CloseSignature();
    void m_RecomputeSignatureState();     // called every frame — re-scans the line to derive depth + arg index
    void m_HandleSignatureDismissKeys();  // Escape only — everything else is handled by m_RecomputeSignatureState
    void m_DrawSignatureTooltip();

    // host popups drawn over the editor (context menus)
    void m_HandleMouseInteractions();  // hover token + right-click menus (uses m_Layout)
    void m_DrawContextMenus();
};

#pragma once

#include <ImGuiPack.h>

#define FIND_POPUP_TEXT_FIELD_LENGTH 128

class CodeEditor
{
public:
	typedef void (*OnFocusedCallback)(int folderViewId);
	typedef void (*OnShowInFolderViewCallback)(const std::string& filePath, int folderViewId);

	CodeEditor(const char* filePath = nullptr,
		int id = -1,
		int createdFromFolderView = -1,
		OnFocusedCallback onFocusedCallback = nullptr,
		OnShowInFolderViewCallback onShowInFolderViewCallback = nullptr);
	~CodeEditor();
    bool init();
    void unit();

    void OnImGui();
	void SetSelection(int startLine, int startChar, int endLine, int endChar);
	const char* GetAssociatedFile();
	void OnFolderViewDeleted(int folderViewId);
	void SetShowDebugPanel(bool value);

	void SetGlslCode(const std::string& vCode);

private:
	void OnReloadCommand();
	void OnLoadFromCommand();
	void OnSaveCommand();

private:
    ImFont* m_CodeFontPtr = nullptr;

	int id = -1;
	int createdFromFolderView = -1;

	OnFocusedCallback onFocusedCallback = nullptr;
	OnShowInFolderViewCallback onShowInFolderViewCallback = nullptr;

	TextEditor editor;
	bool showDebugPanel = false;
	bool hasAssociatedFile = false;
	std::string panelName;
	std::string associatedFile;
	int tabSize = 4;
	float lineSpacing = 1.0f;
	int undoIndexInDisk = 0;

	char ctrlfTextToFind[FIND_POPUP_TEXT_FIELD_LENGTH] = "";
	bool ctrlfCaseSensitive = false;
};
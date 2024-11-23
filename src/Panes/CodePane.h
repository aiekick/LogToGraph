#pragma once

#include <ImGuiPack.h>
#include <frontend/Components/CodeEditor.h>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

class ProjectFile;
class CodePane : public AbstractPane {
private:
    struct CodeSheet {
        CodeEditor codeEditor;
        std::string filepathName;
        std::string title;
        bool wasModified = false;
        bool opened = false;
    };
    std::vector<CodeSheet> m_CodeSheets;

public:
    bool Init() final;
    void Unit() final;
    bool DrawPanes(const uint32_t& vCurrentFrame, bool* vOpened = nullptr, ImGuiContext* vContextPtr = nullptr, void* vUserDatas = nullptr) final;

    void OpenFile(const std::string& vFilePathName, size_t vErrorLine = 0, std::string vErrorMsg = {});

public:  // singleton
    static std::shared_ptr<CodePane> Instance() {
        static std::shared_ptr<CodePane> _instance = std::make_shared<CodePane>();
        return _instance;
    }

public:
    CodePane();                                              // Prevent construction
    CodePane(const CodePane&) = default;                     // Prevent construction by copying
    CodePane& operator=(const CodePane&) { return *this; };  // Prevent assignment
    virtual ~CodePane();                                     // Prevent unwanted destruction};
};

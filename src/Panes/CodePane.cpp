// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include <panes/CodePane.h>
#include <cinttypes>  // printf zu

#include <ezlibs/ezLog.hpp>
#include <ezlibs/ezFile.hpp>

CodePane::CodePane() = default;
CodePane::~CodePane() {
    Unit();
}

bool CodePane::Init() {
    // for avoid a reallocation of the vector for each push/emplace 
    // where the the language Type in each editor got corrupted 
    // because passed by ref
    // 1000 editor is sufficient for our need
    m_CodeSheets.reserve(1000U);

#ifdef _DEBUG
/*    
    auto& sheet = m_CodeSheets.emplace_back();
    sheet.codeEditor.init();
    sheet.filepathName = "C:/Gamedev/gitea/CuiCuiTools/samples/datas/ReportTests/Result_VALGRIND_TOTO.xml";
    sheet.opened = true;
    sheet.wasModified = false;
    sheet.title = "Result_VALGRIND_TOTO.xml";
    auto code = FileHelper::Instance()->LoadFileToString(sheet.filepathName);
    sheet.codeEditor.SetCode(code, TextEditor::LanguageDefinition::Xml());
*/
#endif

    return true;
}

void CodePane::Unit() {
    m_CodeSheets.clear();
}

///////////////////////////////////////////////////////////////////////////////////
//// IMGUI PANE ///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////

bool CodePane::DrawPanes(const uint32_t& /*vCurrentFrame*/, bool* vOpened, ImGuiContext* vContextPtr, void* /*vUserDatas*/) {
    ImGui::SetCurrentContext(vContextPtr);
    bool change = false;
    if (vOpened != nullptr && *vOpened) {
        static ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus  | ImGuiWindowFlags_MenuBar;
        if (ImGui::Begin(GetName().c_str(), vOpened, flags)) {
#ifdef USE_DECORATIONS_FOR_RESIZE_CHILD_WINDOWS
            auto win = ImGui::GetCurrentWindowRead();
            if (win->Viewport->Idx != 0)
                flags |= ImGuiWindowFlags_NoResize;  // | ImGuiWindowFlags_NoTitleBar;
            else
                flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus  | ImGuiWindowFlags_MenuBar;
#endif
            if (ImGui::BeginTabBar("CodePane")) {
                for (auto& sheet : m_CodeSheets) {
                    ImGui::PushID(sheet.filepathName.c_str());
                    if (ImGui::BeginTabItem(sheet.title.c_str(), &sheet.opened)) {
                        sheet.codeEditor.OnImGui();
                        ImGui::EndTabItem();
                    }
                    ImGui::PopID();
                }
                ImGui::EndTabBar();
            }
        }

        ImGui::End();
    }
    return change;
}

void CodePane::OpenFile(const std::string& vFilePathName,  size_t vErrorLine,  std::string vErrorMsg) {
    CodeSheet* existing_code_sheet_ptr = nullptr;
    for (auto& sheet : m_CodeSheets) {
        if (sheet.filepathName == vFilePathName) {
            existing_code_sheet_ptr = &sheet;
            break;
        }
    }

    auto ps = ez::file::parsePathFileName(vFilePathName);
    if (ps.isOk) {
        const auto code = ez::file::loadFileToString(vFilePathName);
        auto type = TextEditor::LanguageDefinition::C();
        if (ps.ext == "cpp" || ps.ext == "hpp") {
            type = TextEditor::LanguageDefinition::Cpp();
        } else if (ps.ext == "c" || ps.ext == "h") {
            type = TextEditor::LanguageDefinition::C();
        } else if (ps.ext == "lua") {
            type = TextEditor::LanguageDefinition::Lua();
        } 
        if (existing_code_sheet_ptr != nullptr) {
            existing_code_sheet_ptr->wasModified = false;
            existing_code_sheet_ptr->opened = true;
            if (!existing_code_sheet_ptr->opened) {
                existing_code_sheet_ptr->wasModified = false;
                existing_code_sheet_ptr->codeEditor.SetCode(code, type);
            }
            existing_code_sheet_ptr->codeEditor.AddErrorMarker(vErrorLine, vErrorMsg);
        } else {
            auto& sheet = m_CodeSheets.emplace_back();
            sheet.codeEditor.init();
            sheet.filepathName = vFilePathName;
            sheet.opened = true;
            sheet.wasModified = false;
            sheet.title = ps.name + "." + ps.ext;
            sheet.codeEditor.SetCode(code, type);
            sheet.codeEditor.AddErrorMarker(vErrorLine, vErrorMsg);
        }
    }
}

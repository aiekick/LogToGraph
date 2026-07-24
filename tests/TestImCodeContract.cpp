// Pins the im::Code model behaviors the CodeEditor port (src/frontend/Components/CodeEditor.cpp)
// relies on. The model API is headless — no ImGui context is required as long as no Render /
// clipboard command is involved. If one of these breaks after an imguipack upgrade, the editor
// features listed in each test name break with it.

#include <imguipack.h>

#include <gtest/gtest.h>

#include <string>
#include <vector>

namespace {

void setContent(im::Code& aEditor, const std::string& aText) {
    aEditor.setText(aText.data(), (uint64_t)aText.size());
}

}  // namespace

///////////////////////////////////////////////////////////////////////////////
// text + cursor model (GetCode / SetCode / MoveCursorTo)
///////////////////////////////////////////////////////////////////////////////

TEST(ImCodeContract, TextRoundTripAndCrlfNormalization) {
    im::Code editor;
    ASSERT_TRUE(editor.init());
    setContent(editor, "line1\r\nline2\nline3");
    EXPECT_EQ(editor.getText(), "line1\nline2\nline3");  // the host line cache splits on '\n' only
}

TEST(ImCodeContract, CursorColumnsAreByteOffsets) {
    im::Code editor;
    ASSERT_TRUE(editor.init());
    setContent(editor, "\tfoo");
    // byte semantics: the tab is one byte — column 4 is the end of "\tfoo" (a visual-column
    // editor would report 4 + tabWidth - 1). The whole completion/signature anchor math of
    // CodeEditor depends on this.
    editor.setCursor(im::Code::Pos{0, 999});
    EXPECT_EQ(editor.getCursor().column, 4);
}

TEST(ImCodeContract, MoveCursorClampsToDocument) {
    im::Code editor;
    ASSERT_TRUE(editor.init());
    setContent(editor, "ab\ncd");
    editor.setCursor(im::Code::Pos{99, 99});
    EXPECT_EQ(editor.getCursor().line, 1);
    EXPECT_EQ(editor.getCursor().column, 2);
}

///////////////////////////////////////////////////////////////////////////////
// selection + insertText — the completion-accept mechanics
// (m_OnCompletionAccepted: select anchor→cursor then insertText(entry))
///////////////////////////////////////////////////////////////////////////////

TEST(ImCodeContract, InsertTextReplacesActiveSelection) {
    im::Code editor;
    ASSERT_TRUE(editor.init());
    // simulate: user typed "ltg.ad" — completion anchor is right after the '.', filter is "ad"
    setContent(editor, "ltg.ad");
    editor.setSelection(im::Code::Range{im::Code::Pos{0, 4}, im::Code::Pos{0, 6}});
    editor.insertText("addSignalValue");
    EXPECT_EQ(editor.getText(), "ltg.addSignalValue");
    EXPECT_EQ(editor.getCursor().column, 18);  // caret lands after the inserted entry
}

TEST(ImCodeContract, SelectWholeLineGivesItsText) {
    im::Code editor;
    ASSERT_TRUE(editor.init());
    // the go-to-line popup selects the target line: setSelection({line,0},{line,len})
    setContent(editor, "aaa\nbbb\nccc");
    editor.setSelection(im::Code::Range{im::Code::Pos{1, 0}, im::Code::Pos{1, 3}});
    EXPECT_EQ(editor.getSelectedText(), "bbb");
    EXPECT_EQ(editor.getCursor().line, 1);  // caret follows the selection end
}

///////////////////////////////////////////////////////////////////////////////
// search — the host find popup drives setSearch/findNext/findPrev
///////////////////////////////////////////////////////////////////////////////

TEST(ImCodeContract, FindNextSelectsAndWraps) {
    im::Code editor;
    ASSERT_TRUE(editor.init());
    setContent(editor, "abc abc abc");
    editor.setSearch("abc", im::Code::FindFlags_None);
    EXPECT_EQ(editor.searchMatchCount(), 3);
    editor.setCursor(im::Code::Pos{0, 0});
    ASSERT_TRUE(editor.findNext());
    EXPECT_EQ(editor.getSelectedText(), "abc");
    EXPECT_EQ(editor.getSelection().start.column, 0);
    ASSERT_TRUE(editor.findNext());
    EXPECT_EQ(editor.getSelection().start.column, 4);
    ASSERT_TRUE(editor.findNext());
    EXPECT_EQ(editor.getSelection().start.column, 8);
    ASSERT_TRUE(editor.findNext());  // wraps
    EXPECT_EQ(editor.getSelection().start.column, 0);
}

TEST(ImCodeContract, CaseInsensitiveFlagMatchesAllCases) {
    im::Code editor;
    ASSERT_TRUE(editor.init());
    // the popup's "Case sensitive" checkbox OFF maps to FindFlags_CaseInsensitive
    setContent(editor, "Foo foo FOO");
    editor.setSearch("foo", im::Code::FindFlags_None);
    EXPECT_EQ(editor.searchMatchCount(), 1);
    editor.setSearch("foo", im::Code::FindFlags_CaseInsensitive);
    EXPECT_EQ(editor.searchMatchCount(), 3);
}

///////////////////////////////////////////////////////////////////////////////
// undo/redo — the Edit menu goes through execute(Command::Undo/Redo)
///////////////////////////////////////////////////////////////////////////////

TEST(ImCodeContract, UndoRedoThroughExecute) {
    im::Code editor;
    ASSERT_TRUE(editor.init());
    setContent(editor, "hello");
    editor.setCursor(im::Code::Pos{0, 5});
    editor.insertText(" world");
    EXPECT_EQ(editor.getText(), "hello world");
    EXPECT_TRUE(editor.execute(im::Code::Command::Undo));
    EXPECT_EQ(editor.getText(), "hello");
    EXPECT_TRUE(editor.execute(im::Code::Command::Redo));
    EXPECT_EQ(editor.getText(), "hello world");
}

///////////////////////////////////////////////////////////////////////////////
// languages — CodePane passes "lua" / "cpp" / "c" by name
///////////////////////////////////////////////////////////////////////////////

TEST(ImCodeContract, LuaLexerColorsKeywords) {
    im::Code editor;
    ASSERT_TRUE(editor.init());
    editor.setLanguage("lua");
    setContent(editor, "local x = 42");
    std::vector<im::Code::Token> tokens;
    editor.getLineTokens(0, tokens);
    ASSERT_FALSE(tokens.empty());
    EXPECT_EQ(tokens[0].startColumn, 0);
    EXPECT_EQ(tokens[0].color, im::Code::Col_Keyword);  // "local"
}

TEST(ImCodeContract, CAndCppNamesResolveToTheSameLexer) {
    im::Code editor;
    ASSERT_TRUE(editor.init());
    std::vector<im::Code::Token> tokens;
    editor.setLanguage("c");  // CodePane uses "c" for .c/.h files
    setContent(editor, "int x;");
    editor.getLineTokens(0, tokens);
    ASSERT_FALSE(tokens.empty());
    EXPECT_EQ(tokens[0].color, im::Code::Col_Type);
    editor.setLanguage("cpp");
    editor.getLineTokens(0, tokens);
    ASSERT_FALSE(tokens.empty());
    EXPECT_EQ(tokens[0].color, im::Code::Col_Type);
}

TEST(ImCodeContract, UnknownLanguageMeansPlainText) {
    im::Code editor;
    ASSERT_TRUE(editor.init());
    editor.setLanguage("");  // CodeEditor::SetCode maps a null language to ""
    setContent(editor, "int x;");
    std::vector<im::Code::Token> tokens;
    editor.getLineTokens(0, tokens);
    EXPECT_TRUE(tokens.empty());
}

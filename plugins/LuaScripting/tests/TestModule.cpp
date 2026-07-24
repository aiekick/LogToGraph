// End-to-end tests of the LuaScripting Module against a fake Ltg::IDatasModel:
// the exact lifecycle the host runs — create / init / load, compileScriptCode,
// callScriptStart / callScriptExec / callScriptEnd — plus error routing (file/line
// extraction feeding the editor markers), the ltg:regex brick through real Lua,
// and the completion/signature catalogs feeding the editor popup.

#include <modules/Module.h>
#include <apis/LtgPluginApi.h>

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <vector>

namespace {

// captures every signal the script pushes — the assertions read these back
struct FakeDatasModel : public Ltg::IDatasModel {
    struct ValueRow {
        std::string category;
        std::string name;
        double epoch = 0.0;
        double value = 0.0;
        std::string desc;
    };
    struct TagRow {
        double epoch = 0.0;
        double r = 0.0, g = 0.0, b = 0.0, a = 0.0;
        std::string name;
        std::string help;
    };
    struct StatusRow {
        std::string category;
        std::string name;
        double epoch = 0.0;
        std::string status;
    };
    struct ZoneRow {
        std::string category;
        std::string name;
        double epoch = 0.0;
        std::string message;
    };

    std::vector<ValueRow> values;
    std::vector<TagRow> tags;
    std::vector<StatusRow> statuses;
    std::vector<ZoneRow> startZones;
    std::vector<ZoneRow> endZones;

    void addSignalTag(double vEpoch, double r, double g, double b, double a, const std::string& vName, const std::string& vHelp) override {
        tags.push_back({vEpoch, r, g, b, a, vName, vHelp});
    }
    void addSignalStatus(const std::string& vCategory, const std::string& vName, double vEpoch, const std::string& vStatus) override {
        statuses.push_back({vCategory, vName, vEpoch, vStatus});
    }
    void addSignalValue(const std::string& vCategory, const std::string& vName, double vEpoch, double vValue, const std::string& vDesc) override {
        values.push_back({vCategory, vName, vEpoch, vValue, vDesc});
    }
    void addSignalStartZone(const std::string& vCategory, const std::string& vName, double vEpoch, const std::string& vStartMsg) override {
        startZones.push_back({vCategory, vName, vEpoch, vStartMsg});
    }
    void addSignalEndZone(const std::string& vCategory, const std::string& vName, double vEpoch, const std::string& vEndMsg) override {
        endZones.push_back({vCategory, vName, vEpoch, vEndMsg});
    }

    const ValueRow* findValue(const std::string& aName) const {
        for (const auto& row : values) {
            if (row.name == aName) {
                return &row;
            }
        }
        return nullptr;
    }
};

class ModuleTest : public ::testing::Test {
protected:
    void SetUp() override {
        m_model = std::make_shared<FakeDatasModel>();
        m_module = std::dynamic_pointer_cast<Ltg::ScriptingModule>(Module::create());
        ASSERT_NE(m_module, nullptr);
        ASSERT_TRUE(m_module->init(nullptr));
        ASSERT_TRUE(m_module->load(m_model));
    }

    void TearDown() override {
        if (m_module != nullptr) {
            m_module->unload();
            m_module->unit();
        }
    }

    // compile + startFile + parse(aBuffer) + endFile, asserting the whole flow succeeds
    void runScript(const std::string& aCode, const std::string& aBuffer) {
        Ltg::ErrorContainer errors;
        ASSERT_TRUE(m_module->compileScriptCode(aCode, errors)) << (errors.empty() ? "" : errors.front().message);
        Ltg::ScriptingDatas datas;
        datas.filepath = "test.log";
        ASSERT_TRUE(m_module->callScriptStart(datas, errors));
        datas.buffer = aBuffer;
        ASSERT_TRUE(m_module->callScriptExec(datas, errors));
        ASSERT_TRUE(m_module->callScriptEnd(datas, errors));
    }

    std::shared_ptr<FakeDatasModel> m_model;
    Ltg::ScriptingModulePtr m_module;
};

}  // namespace

///////////////////////////////////////////////////////////////////////////////
// compile
///////////////////////////////////////////////////////////////////////////////

TEST_F(ModuleTest, CompileValidScriptSucceeds) {
    Ltg::ErrorContainer errors;
    EXPECT_TRUE(m_module->compileScriptCode(
        "function startFile(f) end\n"
        "function parse(b) end\n"
        "function endFile(f) end\n",
        errors));
    EXPECT_TRUE(errors.empty());
}

TEST_F(ModuleTest, CompileScriptMissingParseFails) {
    Ltg::ErrorContainer errors;
    // valid Lua, but the parse(buffer) entry point is missing — rejected without errors
    // (the reason lands in the log, not in the container)
    EXPECT_FALSE(m_module->compileScriptCode(
        "function startFile(f) end\n"
        "function endFile(f) end\n",
        errors));
    EXPECT_TRUE(errors.empty());
}

TEST_F(ModuleTest, CompileSyntaxErrorIsRoutedToTheProjectScriptSheet) {
    Ltg::ErrorContainer errors;
    EXPECT_FALSE(m_module->compileScriptCode(
        "function startFile(f)\n"
        "return (\n"
        "end\n",
        errors));
    ASSERT_EQ(errors.size(), 1u);
    // the host routes errors to editor sheets by file name — MUST stay the shared chunk id
    EXPECT_EQ(errors.front().file, Ltg::sc_PROJECT_SCRIPT_CHUNK);
    EXPECT_GE(errors.front().line, 1u);
    EXPECT_FALSE(errors.front().message.empty());
}

///////////////////////////////////////////////////////////////////////////////
// start / parse / end flow
///////////////////////////////////////////////////////////////////////////////

TEST_F(ModuleTest, ScriptFlowPushesEverySignalKindToTheModel) {
    runScript(R"lua(
function startFile(f) end
function endFile(f) end
function parse(b)
    ltg:addSignalValue("catA", "sigA", 10.5, 42.0)
    ltg:addSignalValue("catB", "sigB", 11.0, 43.5, "desc-b")
    ltg:addSignalTag(12.0, 1.0, 0.5, 0.25, 1.0, "tagName", "tagHelp")
    ltg:addSignalStatus("catC", "sigC", 13.0, "status-c")
    ltg:addSignalStartZone("catD", "sigD", 14.0, "start-d")
    ltg:addSignalEndZone("catD", "sigD", 15.0, "end-d")
end
)lua",
              "one log row");

    ASSERT_EQ(m_model->values.size(), 2u);
    EXPECT_EQ(m_model->values[0].category, "catA");
    EXPECT_EQ(m_model->values[0].name, "sigA");
    EXPECT_DOUBLE_EQ(m_model->values[0].epoch, 10.5);
    EXPECT_DOUBLE_EQ(m_model->values[0].value, 42.0);
    EXPECT_EQ(m_model->values[1].desc, "desc-b");

    ASSERT_EQ(m_model->tags.size(), 1u);
    EXPECT_EQ(m_model->tags[0].name, "tagName");
    EXPECT_DOUBLE_EQ(m_model->tags[0].g, 0.5);

    ASSERT_EQ(m_model->statuses.size(), 1u);
    EXPECT_EQ(m_model->statuses[0].status, "status-c");

    ASSERT_EQ(m_model->startZones.size(), 1u);
    EXPECT_EQ(m_model->startZones[0].message, "start-d");
    ASSERT_EQ(m_model->endZones.size(), 1u);
    EXPECT_EQ(m_model->endZones[0].message, "end-d");
}

TEST_F(ModuleTest, RowIndexAndCountAreVisibleFromLua) {
    m_module->setRowCount(99);
    m_module->setRowIndex(7);
    runScript(R"lua(
function startFile(f) end
function endFile(f) end
function parse(b)
    ltg:addSignalValue("rows", "idx", 0.0, ltg:getRowIndex())
    ltg:addSignalValue("rows", "cnt", 0.0, ltg:getRowCount())
end
)lua",
              "row");
    const auto* idx = m_model->findValue("idx");
    const auto* cnt = m_model->findValue("cnt");
    ASSERT_NE(idx, nullptr);
    ASSERT_NE(cnt, nullptr);
    EXPECT_DOUBLE_EQ(idx->value, 7.0);
    EXPECT_DOUBLE_EQ(cnt->value, 99.0);
}

TEST_F(ModuleTest, RuntimeErrorInParseIsRoutedWithLine) {
    Ltg::ErrorContainer errors;
    ASSERT_TRUE(m_module->compileScriptCode(
        "function startFile(f) end\n"
        "function endFile(f) end\n"
        "function parse(b) error(\"boom\") end\n",
        errors));
    Ltg::ScriptingDatas datas;
    datas.buffer = "row";
    EXPECT_FALSE(m_module->callScriptExec(datas, errors));
    ASSERT_EQ(errors.size(), 1u);
    EXPECT_EQ(errors.front().file, Ltg::sc_PROJECT_SCRIPT_CHUNK);
    EXPECT_EQ(errors.front().line, 3u);  // the error("boom") line
    EXPECT_NE(errors.front().message.find("boom"), std::string::npos);
}

///////////////////////////////////////////////////////////////////////////////
// ltg:regex through real Lua (match / gmatch / gsub / find via the script)
///////////////////////////////////////////////////////////////////////////////

TEST_F(ModuleTest, LuaRegexThroughScript) {
    runScript(R"lua(
local re = ltg:regex("id=(\\d+)")
function startFile(f) end
function endFile(f) end
function parse(b)
    local v = re:match(b)
    if v then ltg:addSignalValue("re", "firstCapture", 0.0, tonumber(v)) end
    local s, n = re:gsub(b, "X")
    ltg:addSignalValue("re", "gsubCount", 0.0, n)
    local count = 0
    for m in re:gmatch(b) do count = count + 1 end
    ltg:addSignalValue("re", "gmatchCount", 0.0, count)
    local a = re:find(b)
    if a then ltg:addSignalValue("re", "findStart", 0.0, a) end
end
)lua",
              "id=123 id=456");

    const auto* firstCapture = m_model->findValue("firstCapture");
    ASSERT_NE(firstCapture, nullptr);
    EXPECT_DOUBLE_EQ(firstCapture->value, 123.0);  // capture group of the FIRST match

    const auto* gsubCount = m_model->findValue("gsubCount");
    ASSERT_NE(gsubCount, nullptr);
    EXPECT_DOUBLE_EQ(gsubCount->value, 2.0);

    const auto* gmatchCount = m_model->findValue("gmatchCount");
    ASSERT_NE(gmatchCount, nullptr);
    EXPECT_DOUBLE_EQ(gmatchCount->value, 2.0);

    const auto* findStart = m_model->findValue("findStart");
    ASSERT_NE(findStart, nullptr);
    EXPECT_DOUBLE_EQ(findStart->value, 1.0);  // Lua-style 1-based
}

TEST_F(ModuleTest, InvalidRegexPatternSurfacesTheRealDiagnostic) {
    Ltg::ErrorContainer errors;
    // the std::regex_error thrown by the LuaRegex ctor must reach the Lua error path with a
    // real message (not the LuaJIT "C++ exception" placeholder) — pinned by
    // SOL_EXCEPTIONS_SAFE_PROPAGATION=0 + the sol2 exception handler
    EXPECT_FALSE(m_module->compileScriptCode(
        "local re = ltg:regex(\"(\")\n"
        "function startFile(f) end\n"
        "function parse(b) end\n"
        "function endFile(f) end\n",
        errors));
    ASSERT_EQ(errors.size(), 1u);
    EXPECT_FALSE(errors.front().message.empty());
    // the LuaJIT placeholder would be exactly "C++ exception" — the real diagnostic is longer
    EXPECT_NE(errors.front().message, "C++ exception");
}

///////////////////////////////////////////////////////////////////////////////
// epoch conversions — difference-based so the host timezone cancels out
///////////////////////////////////////////////////////////////////////////////

TEST_F(ModuleTest, EpochStringRoundTripKeepsDifferences) {
    runScript(R"lua(
function startFile(f) end
function endFile(f) end
function parse(b)
    local sa = ltg:epochToString(1000000000.0, 0.0)
    local sb = ltg:epochToString(1000000010.0, 0.0)
    local ea = ltg:stringToEpoch(sa, 0.0)
    local eb = ltg:stringToEpoch(sb, 0.0)
    ltg:addSignalValue("t", "diff", 0.0, eb - ea)
end
)lua",
              "row");
    const auto* diff = m_model->findValue("diff");
    ASSERT_NE(diff, nullptr);
    EXPECT_DOUBLE_EQ(diff->value, 10.0);
}

///////////////////////////////////////////////////////////////////////////////
// completion + signature catalogs (feed the editor popup / tooltip)
///////////////////////////////////////////////////////////////////////////////

TEST_F(ModuleTest, CompletionCatalogExposesLtgMembers) {
    std::vector<Ltg::CompletionEntry> entries;
    m_module->getCompletionEntries("ltg", entries);
    ASSERT_FALSE(entries.empty());
    bool hasAddSignalValue = false;
    bool hasRegex = false;
    for (const auto& entry : entries) {
        if (entry.name == "addSignalValue") {
            hasAddSignalValue = true;
            EXPECT_EQ(entry.type, "function");
        }
        if (entry.name == "regex") {
            hasRegex = true;
        }
        // sol2/meta noise must stay filtered out of the popup
        EXPECT_NE(entry.name.substr(0, 2), "__");
    }
    EXPECT_TRUE(hasAddSignalValue);
    EXPECT_TRUE(hasRegex);
}

TEST_F(ModuleTest, ProjectScriptGlobalsSurfaceInCompletion) {
    m_module->setProjectScriptCode(
        "helpers = { fooHelper = function() end, barValue = 42 }\n"
        "function parse(b) end\n");
    std::vector<Ltg::CompletionEntry> entries;
    m_module->getCompletionEntries("helpers", entries);
    bool hasFooHelper = false;
    bool hasBarValue = false;
    for (const auto& entry : entries) {
        hasFooHelper = hasFooHelper || (entry.name == "fooHelper");
        hasBarValue = hasBarValue || (entry.name == "barValue");
    }
    EXPECT_TRUE(hasFooHelper);
    EXPECT_TRUE(hasBarValue);
}

TEST_F(ModuleTest, SignatureCatalogKnowsAddSignalValue) {
    Ltg::SignatureInfo signature;
    m_module->getSignatureInfo("ltg", "addSignalValue", signature);
    EXPECT_FALSE(signature.label.empty());
    EXPECT_GE(signature.args.size(), 4u);
}

TEST_F(ModuleTest, UnknownSignatureStaysEmpty) {
    Ltg::SignatureInfo signature;
    m_module->getSignatureInfo("ltg", "doesNotExist", signature);
    EXPECT_TRUE(signature.label.empty());
    EXPECT_TRUE(signature.args.empty());
}

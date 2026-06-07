#pragma once

#include <modules/LuaDatasModel.h>
#include <apis/LtgPluginApi.h>
#include <string>
#include <vector>
#include <memory>
#include <atomic>
#include <cstdint>
#include <unordered_map>
#include <unordered_set>

namespace sol {
class state;
}

struct lua_State;
struct lua_Debug;

class Module : public Ltg::ScriptingModule {
public:
    static Ltg::ScriptingModulePtr create();

private:
    enum class StepMode { None, Into, Over, Out };

private:
    std::unique_ptr<sol::state> m_luaPtr = nullptr;
    Ltg::IDatasModelWeak m_datasModel;
    LuaDatasModelPtr m_luaDatasModelPtr = nullptr;
    // dedicated, permanent sol::state that only holds the bindings + stdlib for autocompletion
    // introspection. lives independently of the analysis state (which is created/destroyed per run).
    std::unique_ptr<sol::state> m_completionLuaPtr = nullptr;
    LuaDatasModelPtr m_completionDatasModelPtr = nullptr;

private:  // debug session state
    Ltg::IScriptDebugHost* m_debugHostPtr = nullptr;  // borrowed, non-owning
    std::atomic<bool> m_pauseRequested{false};
    std::atomic<bool> m_stopRequested{false};
    StepMode m_stepMode{StepMode::None};
    int32_t m_stepBaseDepth{0};
    int32_t m_currentRowIndex{0};
    bool m_debugEnabled{false};
    std::vector<int32_t> m_debugRefs;  // Lua registry refs created during the current pause

public:
    virtual ~Module() = default;
    bool init(Ltg::PluginBridge* vBridgePtr = nullptr) final;
    void unit() final;

    bool load(Ltg::IDatasModelWeak vDatasModel) final;
    void unload() final;
    bool compileScript(const Ltg::ScriptFilePathName& vFilePathName, Ltg::ErrorContainer& vOutErrors) final;
    bool compileScriptCode(const std::string& aCode, Ltg::ErrorContainer& vOutErrors) final;
    bool callScriptStart(const Ltg::ScriptingDatas& vOutDatas, Ltg::ErrorContainer& vOutErrors) final;
    bool callScriptExec(const Ltg::ScriptingDatas& vOutDatas, Ltg::ErrorContainer& vErrors) final;
    bool callScriptEnd(const Ltg::ScriptingDatas& vOutDatas, Ltg::ErrorContainer& vOutErrors) final;

    void setRowIndex(int32_t vRowIndex) final;
    void setRowCount(int32_t vRowCount) final;

    void setProjectScriptCode(const std::string& aCode) final;
    void getCompletionEntries(const std::string& aTarget, std::vector<Ltg::CompletionEntry>& aoEntries) final;
    void getSignatureInfo(const std::string& aTarget, const std::string& aFunctionName, Ltg::SignatureInfo& aoSignature) final;

    // IScriptDebugger — driven by the host
    void enableDebug(Ltg::IScriptDebugHost* apHost) final;
    void disableDebug() final;
    void requestPause() final;
    void requestStop() final;

private:
    static void sLuaHook(lua_State* apLua, lua_Debug* apDebug);
    // Lua-level message handler — invoked by lua_pcall BEFORE the stack unwinds when the called
    // function (or anything it calls) raises a Lua error (string.match(nil), nil:method(), etc.).
    // Distinct from the sol2 exception_handler which only fires for C++ exceptions thrown by
    // bindings. Recovers `this` from the lua_State registry (same key as the line hook), checks
    // shouldPauseOnError, and calls m_pauseOnError synchronously while the throwing frame is
    // still alive — so locals/upvalues/call-stack are inspectable in the debug UI.
    static int sLuaErrorHandler(lua_State* apLua);
    void m_onHook(lua_State* apLua, lua_Debug* apDebug);
    bool m_shouldBreak(lua_State* apLua, int32_t aLine);
    Ltg::DebugState m_buildState(lua_State* apLua, lua_Debug* apDebug);
    // Build a paused-on-error snapshot from the sol2 exception_handler. Same call-stack + globals
    // walk as m_buildState, but line/source come from the parsed error message (which carries the
    // Lua-reported throw site), and the state is flagged errorPause = true so the host can auto-set
    // a breakpoint at that line.
    Ltg::DebugState m_buildErrorState(lua_State* apLua, const std::string& aErrorMessage);
    // Shared helper — walks the Lua call stack (innermost first, capped at 64) and the _G table,
    // filling state.callStack and state.globals. Used by both m_buildState and m_buildErrorState.
    void m_fillCallStackAndGlobals(lua_State* apLua, Ltg::DebugState& aoState);
    // Drives the onPause -> waitAction loop and applies the returned command. Used by both the
    // line-hook path (m_onHook) and the exception_handler path (m_pauseOnError).
    void m_runPauseLoop(lua_State* apLua, Ltg::DebugState aState);
    // Called from the sol2 exception_handler when shouldPauseOnError() is true.
    void m_pauseOnError(lua_State* apLua, const std::string& aErrorMessage);
    void m_applyCommand(lua_State* apLua, Ltg::DebugCommand aCommand);
    int32_t m_makeRef(lua_State* apLua, int32_t aIndex);
    Ltg::DebugVar m_makeVar(lua_State* apLua, int32_t aIndex, const std::string& aName, const std::string& aKeyType);
    std::vector<Ltg::DebugVar> m_expandRef(lua_State* apLua, int32_t aRef);
    Ltg::EvalResult m_evalExpression(lua_State* apLua, lua_Debug* apDebug, const std::string& aExpression);
    void m_releaseDebugRefs(lua_State* apLua);

    void m_ensureCompletionState();  // lazy init of m_completionLuaPtr on first getCompletionEntries call
    static void m_iterateLuaTable(lua_State* apLua, int aTableIndex, std::vector<Ltg::CompletionEntry>& aoEntries);

    // keys we currently expose as user globals in the completion state's _G — wiped before each
    // setProjectScriptCode refresh so removed top-level definitions stop appearing in autocomplete.
    std::unordered_set<std::string> m_completionUserGlobals;
};

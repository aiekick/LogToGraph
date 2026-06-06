#pragma once

#include <modules/LuaDatasModel.h>
#include <apis/LtgPluginApi.h>
#include <string>
#include <vector>
#include <memory>
#include <atomic>
#include <cstdint>
#include <unordered_map>

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
    bool callScriptStart(Ltg::ErrorContainer& vOutErrors) final;
    bool callScriptExec(const Ltg::ScriptingDatas& vOutDatas, Ltg::ErrorContainer& vErrors) final;
    bool callScriptEnd(Ltg::ErrorContainer& vOutErrors) final;

    void setRowIndex(int32_t vRowIndex) final;
    void setRowCount(int32_t vRowCount) final;

    void getCompletionEntries(const std::string& aTarget, std::vector<Ltg::CompletionEntry>& aoEntries) final;

    // IScriptDebugger — driven by the host
    void enableDebug(Ltg::IScriptDebugHost* apHost) final;
    void disableDebug() final;
    void requestPause() final;
    void requestStop() final;

private:
    static void sLuaHook(lua_State* apLua, lua_Debug* apDebug);
    void m_onHook(lua_State* apLua, lua_Debug* apDebug);
    bool m_shouldBreak(lua_State* apLua, int32_t aLine);
    Ltg::DebugState m_buildState(lua_State* apLua, lua_Debug* apDebug);
    void m_applyCommand(lua_State* apLua, Ltg::DebugCommand aCommand);
    int32_t m_makeRef(lua_State* apLua, int32_t aIndex);
    Ltg::DebugVar m_makeVar(lua_State* apLua, int32_t aIndex, const std::string& aName, const std::string& aKeyType);
    std::vector<Ltg::DebugVar> m_expandRef(lua_State* apLua, int32_t aRef);
    Ltg::EvalResult m_evalExpression(lua_State* apLua, lua_Debug* apDebug, const std::string& aExpression);
    void m_releaseDebugRefs(lua_State* apLua);

    void m_ensureCompletionState();  // lazy init of m_completionLuaPtr on first getCompletionEntries call
    static void m_iterateLuaTable(lua_State* apLua, int aTableIndex, std::vector<Ltg::CompletionEntry>& aoEntries);
};

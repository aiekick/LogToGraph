// sol2 config (SOL_ALL_SAFETIES_ON, SOL_EXCEPTIONS_SAFE_PROPAGATION) is now
// injected by the plugin's CMakeLists so it reaches every TU before any
// <sol/sol.hpp> include.
#include "Module.h"
#include <ezlibs/ezFile.hpp>
#include <ezlibs/ezTime.hpp>
#include <ezlibs/ezLog.hpp>
#include <imguipack.h>
#include <exception>
#include <chrono>
#include <ctime>
#include <cstdio>
#include <string>
#include <unordered_map>

#include <lua.hpp>
#include <sol/sol.hpp>

// recover the Module owning a lua_State from inside the C hook; only the single
// parsing worker thread touches this map (enableDebug / hook / unload all run there)
std::unordered_map<lua_State*, Module*> Module::s_modulesByState;

namespace {

// Stringify a value on the Lua stack WITHOUT luaL_tolstring (absent from LuaJIT 5.1).
// The type is checked first so lua_tostring is only called on real strings (it would
// otherwise coerce a number in place and disturb the stack inside the hook).
std::string luaValueToString(lua_State* apLua, int32_t aIndex) {
    const int32_t valueType = lua_type(apLua, aIndex);
    switch (valueType) {
        case LUA_TNIL: return "nil";
        case LUA_TBOOLEAN: return lua_toboolean(apLua, aIndex) != 0 ? "true" : "false";
        case LUA_TNUMBER: {
            char buffer[64];
            snprintf(buffer, sizeof(buffer), "%.14g", lua_tonumber(apLua, aIndex));
            return buffer;
        }
        case LUA_TSTRING: return std::string(lua_tostring(apLua, aIndex));
        default: {
            char buffer[64];
            snprintf(buffer, sizeof(buffer), "%s: %p", lua_typename(apLua, valueType), lua_topointer(apLua, aIndex));
            return buffer;
        }
    }
}

int32_t luaStackDepth(lua_State* apLua) {
    int32_t depth = 0;
    lua_Debug frameInfo;
    while (lua_getstack(apLua, depth, &frameInfo) != 0) {
        ++depth;
    }
    return depth;
}

}  // namespace

Ltg::ScriptingModulePtr Module::create(const SettingsWeak& vSettings) {
    assert(!vSettings.expired());
    auto res = std::make_shared<Module>();
    res->m_settings = vSettings;
    if (!res->init()) {
        res.reset();
    }
    return res;
}

bool Module::init(Ltg::PluginBridge* vBridgePtr) {
    return true;
}

void Module::unit() {}

bool Module::load(Ltg::IDatasModelWeak vDatasModel) {
    try {
        m_datasModel = vDatasModel;

        m_luaPtr = std::make_unique<sol::state>();

        // By default, sol2 converts C++ exceptions thrown from bound functions into a
        // generic "C++ exception" lua_error — the actual std::exception::what() is lost.
        // This handler forwards the real what() string back to Lua, so script error messages
        // surface the original throw text ("Invalid date format", etc.) instead.
        m_luaPtr->set_exception_handler(
            [](lua_State* L, sol::optional<const std::exception&> maybe_exception, sol::string_view description) {
                if (maybe_exception) {
                    const std::exception& ex = *maybe_exception;
                    return sol::stack::push(L, ex.what());
                }
                return sol::stack::push(L, description);
            });

        m_luaPtr->open_libraries(sol::lib::base);
        m_luaPtr->open_libraries(sol::lib::package);
        m_luaPtr->open_libraries(sol::lib::coroutine);
        m_luaPtr->open_libraries(sol::lib::string);
        m_luaPtr->open_libraries(sol::lib::os);
        m_luaPtr->open_libraries(sol::lib::math);
        m_luaPtr->open_libraries(sol::lib::table);
        m_luaPtr->open_libraries(sol::lib::debug);
        m_luaPtr->open_libraries(sol::lib::bit32);
        m_luaPtr->open_libraries(sol::lib::io);
        m_luaPtr->open_libraries(sol::lib::ffi);
        m_luaPtr->open_libraries(sol::lib::jit);

        m_luaPtr->set_function("print", [](sol::variadic_args args) {
            std::string res;
            for (auto arg : args) {
                res += arg.get<std::string>() + " ";  // Convertir chaque argument en string
            }
            if (!res.empty()) {
                res.pop_back();
                LogVarLightInfo("Lua: %s", res.c_str());
            }
        });

        // clang-format off
        m_luaPtr->new_usertype<LuaDatasModel>(
            "LuaDatasModel", sol::constructors<std::shared_ptr<LuaDatasModel>()>(),
            "stringToEpoch", &LuaDatasModel::luaModuleStringToEpoch,
            "epochToString", &LuaDatasModel::luaModuleEpochToString,
            "addSignalTag", &LuaDatasModel::luaModuleAddSignalTag,
            "addSignalStatus", &LuaDatasModel::luaModuleAddSignalStatus,
            "addSignalValue",sol::overload(
                &LuaDatasModel::luaModuleAddSignalValue,
                &LuaDatasModel::luaModuleAddSignalValueWithDesc),
            "addSignalStartZone", &LuaDatasModel::luaModuleAddSignalStartZone,
            "addSignalEndZone", &LuaDatasModel::luaModuleAddSignalEndZone,
            "logInfo", &LuaDatasModel::luaModuleLogInfo,
            "logWarning", &LuaDatasModel::luaModuleLogWarning,
            "logError", &LuaDatasModel::luaModuleLogError,
            "logDebug", &LuaDatasModel::luaModuleLogDebug,
            "getRowIndex", &LuaDatasModel::luaModuleGetRowIndex,
            "getRowCount", &LuaDatasModel::luaModuleGetRowCount
        );
        // clang-format on

        (*m_luaPtr)["ltg"] = m_luaDatasModelPtr = LuaDatasModel::create(vDatasModel);

        return (m_luaPtr != nullptr) && (m_luaDatasModelPtr != nullptr) && (!m_datasModel.expired());
    } catch (std::exception& ex) {
        LogVarError("Fail to init Lua : %s", ex.what());
        return false;
    } catch (...) {
        return false;
    }
}

void Module::unload() {
    if (m_luaPtr != nullptr) {
        // drop any debug hook binding before the lua_State is destroyed
        s_modulesByState.erase(m_luaPtr->lua_state());
    }
    m_luaPtr.reset();
}

bool Module::compileScript(const Ltg::ScriptFilePathName& vFilePathName, Ltg::ErrorContainer& vOutErrors) {
    try {
        m_luaPtr->script_file(vFilePathName);
        bool res = true;
        sol::function parse = (*m_luaPtr)["parse"];
        if (!parse.valid()) {
            LogVarLightError("Lua: %s", "the lua function parse(buffer) is missing");
            res = false;
        }
        res &= callScriptStart(vOutErrors);
        res &= callScriptEnd(vOutErrors);
        return res;
    } catch (const sol::error& ex) {
        LogVarError("Lua: Error in the Lua script : %s", ex.what());
    } catch (const std::exception& ex) {
        LogVarError("Lua: Error in the Lua script : %s", ex.what());
    } catch (...) {
        LogVarError("Lua: %s", "Unknown error in the Lua script");
    }
    return false;
}

bool Module::callScriptStart(Ltg::ErrorContainer& vOutErrors) {
    sol::protected_function startFile = (*m_luaPtr)["startFile"];
    if (!startFile.valid()) {
        LogVarLightError("Lua: %s", "the lua function startFile() is missing");
        return false;
    }
    sol::protected_function_result result = startFile();
    if (!result.valid()) {
        sol::error err = result;
        LogVarLightError("Lua: error in startFile func call : %s", err.what());
        return false;
    }
    return true;
}

bool Module::callScriptExec(const Ltg::ScriptingDatas& vOutDatas, Ltg::ErrorContainer& vErrors) {
    sol::protected_function parse = (*m_luaPtr)["parse"];
    if (!parse.valid()) {
        LogVarLightError("Lua: %s", "the lua function parse(buffer) is missing");
        return false;
    }
    sol::protected_function_result result = parse(vOutDatas.buffer);
    if (!result.valid()) {
        sol::error err = result;
        LogVarLightError("Lua: error in parse func call : %s", err.what());
        return false;
    }
    return true;
}

bool Module::callScriptEnd(Ltg::ErrorContainer& vOutErrors) {
    sol::protected_function endFile = (*m_luaPtr)["endFile"];
    if (!endFile.valid()) {
        LogVarLightError("%s", "the lua function endFile() is missing");
        return false;
    }
    sol::protected_function_result result = endFile();
    if (!result.valid()) {
        sol::error err = result;
        LogVarLightError("Lua: error in endFile func call : %s", err.what());
        return false;
    }
    return true;
}

void Module::setRowIndex(int32_t vRowIndex) {
    m_currentRowIndex = vRowIndex;  // exposed in DebugState so the user knows the log row
    if (m_luaDatasModelPtr != nullptr) {
        m_luaDatasModelPtr->setRowIndex(vRowIndex);
    }
}

void Module::setRowCount(int32_t vRowCount) {
    if (m_luaDatasModelPtr != nullptr) {
        m_luaDatasModelPtr->setRowCount(vRowCount);
    }
}

///////////////////////////////////////////////////////////////////////////////////
//// DEBUGGER (IScriptDebugger) /////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////

void Module::enableDebug(Ltg::IScriptDebugHost* apHost) {
    if (m_luaPtr == nullptr) {
        return;
    }
    m_debugHostPtr = apHost;
    m_stepMode = StepMode::None;
    m_pauseRequested = false;
    m_stopRequested = false;
    lua_State* luaStatePtr = m_luaPtr->lua_state();
    s_modulesByState[luaStatePtr] = this;
    // disable the JIT so the line hook fires on every line during the debug session
    m_luaPtr->safe_script("if jit and jit.off then jit.off() end", sol::script_pass_on_error);
    lua_sethook(luaStatePtr, &Module::sLuaHook, LUA_MASKLINE, 0);
    m_debugEnabled = true;
}

void Module::disableDebug() {
    if (m_luaPtr == nullptr) {
        return;
    }
    lua_State* luaStatePtr = m_luaPtr->lua_state();
    lua_sethook(luaStatePtr, nullptr, 0, 0);
    s_modulesByState.erase(luaStatePtr);
    m_luaPtr->safe_script("if jit and jit.on then jit.on() end", sol::script_pass_on_error);
    m_debugHostPtr = nullptr;
    m_debugEnabled = false;
}

void Module::setBreakpoints(const Ltg::BreakpointLines& aLines) {
    m_breakpoints = aLines;
}

void Module::requestPause() {
    m_pauseRequested = true;
}

void Module::requestStop() {
    m_stopRequested = true;
}

void Module::sLuaHook(lua_State* apLua, lua_Debug* apDebug) {
    const auto moduleIt = s_modulesByState.find(apLua);
    if (moduleIt != s_modulesByState.end() && moduleIt->second != nullptr) {
        moduleIt->second->m_onHook(apLua, apDebug);
    }
}

void Module::m_onHook(lua_State* apLua, lua_Debug* apDebug) {
    if (m_debugHostPtr == nullptr) {
        return;
    }
    if (apDebug->event != LUA_HOOKLINE) {
        return;
    }
    if (m_stopRequested) {
        // unconditional abort; do not go through onPause (which would block again)
        luaL_error(apLua, "execution stopped by the debugger");  // longjmp, never returns
        return;
    }
    lua_getinfo(apLua, "Sl", apDebug);  // fill currentline + short_src
    const int32_t line = static_cast<int32_t>(apDebug->currentline);
    if (!m_shouldBreak(apLua, line)) {
        return;
    }
    Ltg::DebugState state = m_buildState(apLua, apDebug);
    const Ltg::DebugCommand command = m_debugHostPtr->onPause(state);  // BLOCKS the worker thread
    m_applyCommand(apLua, command);
}

bool Module::m_shouldBreak(lua_State* apLua, int32_t aLine) {
    if (m_pauseRequested) {
        return true;
    }
    if (m_breakpoints.find(aLine) != m_breakpoints.end()) {
        return true;
    }
    switch (m_stepMode) {
        case StepMode::Into: return true;
        case StepMode::Over: return luaStackDepth(apLua) <= m_stepBaseDepth;
        case StepMode::Out: return luaStackDepth(apLua) < m_stepBaseDepth;
        case StepMode::None:
        default: return false;
    }
}

Ltg::DebugState Module::m_buildState(lua_State* apLua, lua_Debug* apDebug) {
    Ltg::DebugState state;
    state.logRowIndex = m_currentRowIndex;
    state.line = static_cast<int32_t>(apDebug->currentline);
    state.sourceFile = apDebug->short_src;

    // call stack, innermost first, capped to keep the snapshot small
    lua_Debug frameInfo;
    for (int32_t level = 0; level < 64 && lua_getstack(apLua, level, &frameInfo) != 0; ++level) {
        lua_getinfo(apLua, "nSl", &frameInfo);
        Ltg::DebugFrame frame;
        frame.function = (frameInfo.name != nullptr) ? frameInfo.name : "";
        frame.source = frameInfo.short_src;
        frame.line = static_cast<int32_t>(frameInfo.currentline);
        state.callStack.push_back(frame);
    }

    // locals of the current frame (skip internal temporaries named like "(temporary)")
    int32_t localIndex = 1;
    const char* localName = nullptr;
    while ((localName = lua_getlocal(apLua, apDebug, localIndex)) != nullptr) {
        if (localName[0] != '(') {
            Ltg::DebugVar var;
            var.name = localName;
            var.value = luaValueToString(apLua, -1);
            var.typeName = lua_typename(apLua, lua_type(apLua, -1));
            state.locals.push_back(var);
        }
        lua_pop(apLua, 1);
        ++localIndex;
    }

    // upvalues of the current function (lua_getinfo "f" pushes the running function)
    if (lua_getinfo(apLua, "f", apDebug) != 0) {
        const int32_t functionIndex = lua_gettop(apLua);
        int32_t upvalueIndex = 1;
        const char* upvalueName = nullptr;
        while ((upvalueName = lua_getupvalue(apLua, functionIndex, upvalueIndex)) != nullptr) {
            if (upvalueName[0] != '\0') {
                Ltg::DebugVar var;
                var.name = upvalueName;
                var.value = luaValueToString(apLua, -1);
                var.typeName = lua_typename(apLua, lua_type(apLua, -1));
                state.upvalues.push_back(var);
            }
            lua_pop(apLua, 1);  // pop the upvalue
            ++upvalueIndex;
        }
        lua_pop(apLua, 1);  // pop the function pushed by lua_getinfo "f"
    }

    return state;
}

void Module::m_applyCommand(lua_State* apLua, Ltg::DebugCommand aCommand) {
    m_pauseRequested = false;
    switch (aCommand) {
        case Ltg::DebugCommand::Continue: m_stepMode = StepMode::None; break;
        case Ltg::DebugCommand::StepInto: m_stepMode = StepMode::Into; break;
        case Ltg::DebugCommand::StepOver:
            m_stepMode = StepMode::Over;
            m_stepBaseDepth = luaStackDepth(apLua);
            break;
        case Ltg::DebugCommand::StepOut:
            m_stepMode = StepMode::Out;
            m_stepBaseDepth = luaStackDepth(apLua);
            break;
        case Ltg::DebugCommand::Stop:
            m_stepMode = StepMode::None;
            m_stopRequested = true;
            luaL_error(apLua, "execution stopped by the debugger");  // longjmp, never returns
            break;
    }
}

// sol2 config (SOL_ALL_SAFETIES_ON, SOL_EXCEPTIONS_SAFE_PROPAGATION) is now
// injected by the plugin's CMakeLists so it reaches every TU before any
// <sol/sol.hpp> include.
#include "Module.h"
#include <ezlibs/ezFile.hpp>
#include <ezlibs/ezTime.hpp>
#include <ezlibs/ezLog.hpp>
#include <algorithm>
#include <exception>
#include <chrono>
#include <ctime>
#include <cstdio>
#include <string>
#include <unordered_map>
#include <unordered_set>

// boost::regex (vendored by imguipack via USE_IMGUI_COLOR_TEXT_EDIT) — same engine the host will use
// when the future shared ltg:regex(...) brick lands. linked through the boost_regex CMake target.
#include <boost/regex.hpp>

#include <lua.hpp>
#include <sol/sol.hpp>

namespace {

// Registry key binding a lua_State to its owning Module, so the C line hook can recover the
// Module from the lua_State it is handed (lua_sethook carries no user data). The binding lives
// in the state's own registry: it works for several states at once and for coroutines, and is
// freed with the state — so there is no process-wide static to be destroyed (and crash) at the
// plugin DLL's unload.
const char* const kDebugModuleRegistryKey = "ltg_debug_module";

// Parse a Lua/sol2 error message and extract (file, line). Sol2 wraps an in-memory chunk as
// `[string "NAME"]:LINE: MSG`; a file-based script reads `NAME:LINE: MSG`. On a match, the full
// original message is kept as `aoErr.message` (it stays useful for the tooltip even if regex only
// found a prefix). On no match, the message lands on `aFallbackChunk:0` so the host can still display
// it (just unanchored). Returns true on a regex match.
bool parseLuaError(const std::string& aMsg, const std::string& aFallbackChunk, Ltg::ScriptingError& aoErr) {
    // delimiter `re(...)re` mandatory: the chunkRe pattern contains a literal `)"` (the `+)"` after
    // the `[^"]+` group), which would otherwise close an empty-delimiter R"(...)" prematurely.
    static const boost::regex chunkRe(R"re(\[string\s+"([^"]+)"\]:(\d+):\s*(.*))re");
    static const boost::regex plainRe(R"re(([^:\[\]\s]+):(\d+):\s*(.*))re");
    boost::smatch match;
    aoErr.message = aMsg;
    if (boost::regex_search(aMsg, match, chunkRe)) {
        aoErr.file = match[1].str();
        aoErr.line = static_cast<size_t>(std::stoul(match[2].str()));
        return true;
    }
    if (boost::regex_search(aMsg, match, plainRe)) {
        aoErr.file = match[1].str();
        aoErr.line = static_cast<size_t>(std::stoul(match[2].str()));
        return true;
    }
    aoErr.file = aFallbackChunk;
    aoErr.line = 0;
    return false;
}

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

// cap on children read per lazy expansion (one level), to keep a single expand cheap
constexpr int32_t kMaxExpandChildren = 1000;

// stringify a table key without coercing it on the stack (would break lua_next)
std::string luaKeyToString(lua_State* apLua, int32_t aIndex) {
    const int32_t keyType = lua_type(apLua, aIndex);
    switch (keyType) {
        case LUA_TSTRING: return std::string(lua_tostring(apLua, aIndex));
        case LUA_TNUMBER: {
            char buffer[64];
            snprintf(buffer, sizeof(buffer), "[%.14g]", lua_tonumber(apLua, aIndex));
            return buffer;
        }
        case LUA_TBOOLEAN: return lua_toboolean(apLua, aIndex) != 0 ? "[true]" : "[false]";
        default: {
            char buffer[64];
            snprintf(buffer, sizeof(buffer), "[%s]", lua_typename(apLua, keyType));
            return buffer;
        }
    }
}

}  // namespace

Ltg::ScriptingModulePtr Module::create() {
    auto res = std::make_shared<Module>();
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
    // the registry-held debug binding (if any) dies with the lua_State
    m_luaPtr.reset();
}

bool Module::compileScript(const Ltg::ScriptFilePathName& vFilePathName, Ltg::ErrorContainer& vOutErrors) {
    try {
        m_luaPtr->script_file(vFilePathName);
        // validate the required entry points exist — DO NOT call them here (see compileScriptCode
        // for the rationale: ScriptingEngine's loop calls them at the right time per source file).
        bool res = true;
        sol::function parse = (*m_luaPtr)["parse"];
        if (!parse.valid()) {
            LogVarLightError("Lua: %s", "the lua function parse(buffer) is missing");
            res = false;
        }
        sol::function startFile = (*m_luaPtr)["startFile"];
        if (!startFile.valid()) {
            LogVarLightError("Lua: %s", "the lua function startFile() is missing");
            res = false;
        }
        sol::function endFile = (*m_luaPtr)["endFile"];
        if (!endFile.valid()) {
            LogVarLightError("Lua: %s", "the lua function endFile() is missing");
            res = false;
        }
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

bool Module::compileScriptCode(const std::string& aCode, Ltg::ErrorContainer& vOutErrors) {
    try {
        // pass an explicit chunk name so sol2's error messages identify our project script
        // (otherwise sol2 uses the first line of code as the chunk name, which breaks routing).
        m_luaPtr->script(aCode, Ltg::sc_PROJECT_SCRIPT_CHUNK);  // compile + run the chunk (defines parse/startFile/endFile)
        // validate the required entry points exist — DO NOT call them here. ScriptingEngine's main
        // loop is responsible for calling startFile/parse/endFile at the proper times; calling them
        // here would double-execute user code (duplicate logs / state init).
        bool res = true;
        sol::function parse = (*m_luaPtr)["parse"];
        if (!parse.valid()) {
            LogVarLightError("Lua: %s", "the lua function parse(buffer) is missing");
            res = false;
        }
        sol::function startFile = (*m_luaPtr)["startFile"];
        if (!startFile.valid()) {
            LogVarLightError("Lua: %s", "the lua function startFile() is missing");
            res = false;
        }
        sol::function endFile = (*m_luaPtr)["endFile"];
        if (!endFile.valid()) {
            LogVarLightError("Lua: %s", "the lua function endFile() is missing");
            res = false;
        }
        return res;
    } catch (const sol::error& ex) {
        Ltg::ScriptingError err;
        parseLuaError(ex.what(), Ltg::sc_PROJECT_SCRIPT_CHUNK, err);
        vOutErrors.push_back(err);
        LogVarError("Lua: Error in the Lua script : %s", ex.what());
    } catch (const std::exception& ex) {
        Ltg::ScriptingError err;
        parseLuaError(ex.what(), Ltg::sc_PROJECT_SCRIPT_CHUNK, err);
        vOutErrors.push_back(err);
        LogVarError("Lua: Error in the Lua script : %s", ex.what());
    } catch (...) {
        Ltg::ScriptingError err;
        err.file = Ltg::sc_PROJECT_SCRIPT_CHUNK;
        err.message = "Unknown error in the Lua script";
        vOutErrors.push_back(err);
        LogVarError("Lua: %s", "Unknown error in the Lua script");
    }
    return false;
}

void Module::m_ensureCompletionState() {
    if (m_completionLuaPtr != nullptr) {
        return;
    }
    // dedicated state that mirrors the analysis state's bindings but never executes user code.
    // we instantiate LuaDatasModel directly (skipping the create() factory which would null it out
    // when no IDatasModel is bound) because we only need the usertype methods to be REGISTERED in the
    // metatable for introspection — none of them is ever actually called from the completion state.
    m_completionLuaPtr = std::unique_ptr<sol::state>(new sol::state());
    m_completionLuaPtr->open_libraries(sol::lib::base);
    m_completionLuaPtr->open_libraries(sol::lib::package);
    m_completionLuaPtr->open_libraries(sol::lib::coroutine);
    m_completionLuaPtr->open_libraries(sol::lib::string);
    m_completionLuaPtr->open_libraries(sol::lib::os);
    m_completionLuaPtr->open_libraries(sol::lib::math);
    m_completionLuaPtr->open_libraries(sol::lib::table);
    m_completionLuaPtr->open_libraries(sol::lib::debug);
    m_completionLuaPtr->open_libraries(sol::lib::bit32);
    m_completionLuaPtr->open_libraries(sol::lib::io);
    m_completionLuaPtr->open_libraries(sol::lib::ffi);
    m_completionLuaPtr->open_libraries(sol::lib::jit);

    // clang-format off
    m_completionLuaPtr->new_usertype<LuaDatasModel>(
        "LuaDatasModel", sol::constructors<std::shared_ptr<LuaDatasModel>()>(),
        "stringToEpoch", &LuaDatasModel::luaModuleStringToEpoch,
        "epochToString", &LuaDatasModel::luaModuleEpochToString,
        "addSignalTag", &LuaDatasModel::luaModuleAddSignalTag,
        "addSignalStatus", &LuaDatasModel::luaModuleAddSignalStatus,
        "addSignalValue", sol::overload(
            &LuaDatasModel::luaModuleAddSignalValue,
            &LuaDatasModel::luaModuleAddSignalValueWithDesc),
        "addSignalStartZone", &LuaDatasModel::luaModuleAddSignalStartZone,
        "addSignalEndZone", &LuaDatasModel::luaModuleAddSignalEndZone,
        "logInfo", &LuaDatasModel::luaModuleLogInfo,
        "logWarning", &LuaDatasModel::luaModuleLogWarning,
        "logError", &LuaDatasModel::luaModuleLogError,
        "logDebug", &LuaDatasModel::luaModuleLogDebug,
        "getRowIndex", &LuaDatasModel::luaModuleGetRowIndex,
        "getRowCount", &LuaDatasModel::luaModuleGetRowCount);
    // clang-format on

    // empty stub instance: ctor is public, no IDatasModel is required for introspection.
    m_completionDatasModelPtr = std::make_shared<LuaDatasModel>();
    (*m_completionLuaPtr)["ltg"] = m_completionDatasModelPtr;
}

// Filter out keys that are Lua-internal metamethods (`__name`, `__index`, `__gc`, `__eq`,
// `__pairs`, `__newindex`, `__type`, ...) and sol2-internal helpers (`class_check`, `class_cast`,
// `new`). These appear in the metatable iteration but are noise from the user's POV — they
// don't help write Lua, they expose implementation details.
static bool s_isCompletionNoise(const std::string& aName) {
    if (aName.size() >= 2 && aName[0] == '_' && aName[1] == '_') {
        return true;  // any `__*` metamethod
    }
    if (aName.compare(0, 6, "class_") == 0) {
        return true;  // sol2 inheritance bookkeeping (class_check, class_cast)
    }
    if (aName == "new") {
        return true;  // sol2 default constructor binding — never useful through `:method()` autocompletion
    }
    return false;
}

void Module::m_iterateLuaTable(lua_State* apLua, int aTableIndex, std::vector<Ltg::CompletionEntry>& aoEntries) {
    // aTableIndex must be an absolute stack index (lua_next manipulates the stack, breaking relative refs).
    lua_pushnil(apLua);
    while (lua_next(apLua, aTableIndex) != 0) {
        // -2 is the key, -1 is the value
        if (lua_type(apLua, -2) == LUA_TSTRING) {
            std::string keyName = lua_tostring(apLua, -2);
            if (!s_isCompletionNoise(keyName)) {
                Ltg::CompletionEntry entry;
                entry.name = std::move(keyName);
                entry.type = lua_typename(apLua, lua_type(apLua, -1));
                aoEntries.push_back(std::move(entry));
            }
        }
        lua_pop(apLua, 1);  // pop value, keep key for next iter
    }
}

void Module::getCompletionEntries(const std::string& aTarget, std::vector<Ltg::CompletionEntry>& aoEntries) {
    if (aTarget.empty()) {
        return;
    }
    m_ensureCompletionState();
    lua_State* L = m_completionLuaPtr->lua_state();
    const int topBefore = lua_gettop(L);

    lua_getglobal(L, aTarget.c_str());
    const int targetType = lua_type(L, -1);
    if (targetType == LUA_TTABLE) {
        // plain table (Lua stdlib namespace, user table) — iterate directly
        m_iterateLuaTable(L, lua_gettop(L), aoEntries);
    } else if (targetType == LUA_TUSERDATA) {
        // sol2 usertype — methods are stored in the metatable's __index field, hop through it
        if (lua_getmetatable(L, -1) != 0) {
            lua_getfield(L, -1, "__index");
            if (lua_type(L, -1) == LUA_TTABLE) {
                m_iterateLuaTable(L, lua_gettop(L), aoEntries);
            }
            lua_pop(L, 2);  // __index + metatable
        }
    }
    lua_settop(L, topBefore);

    // alphabetic order — lua_next returns hash-table order which has no stable meaning to the user.
    std::sort(aoEntries.begin(), aoEntries.end(), [](const Ltg::CompletionEntry& aLhs, const Ltg::CompletionEntry& aRhs) {
        return aLhs.name < aRhs.name;
    });
}

namespace {

// Hand-maintained signature catalog for the Lua plugin. Must be kept in sync with the bindings
// in Module::load (and m_ensureCompletionState). For overloaded methods we list the widest
// variant — the host shows a single line in v1, no overload picker yet. v1 covers `ltg:`
// methods only; `math.*` / `string.*` curated entries will land next.
struct CatalogArg {
    const char* name;
    const char* type;
};
struct SignatureEntry {
    const char* target;        // "" for globals
    const char* functionName;
    std::vector<CatalogArg> args;
};

const std::vector<SignatureEntry>& s_signatureCatalog() {
    static const std::vector<SignatureEntry> catalog = {
        // ltg: usertype methods (cf. new_usertype<LuaDatasModel> in Module::load)
        {"ltg", "stringToEpoch",      {{"dateTime","string"}, {"hourOffset","number"}}},
        {"ltg", "epochToString",      {{"epochTime","number"}, {"hourOffset","number"}}},
        {"ltg", "addSignalTag",       {{"epoch","number"}, {"r","number"}, {"g","number"}, {"b","number"}, {"a","number"}, {"name","string"}, {"help","string"}}},
        {"ltg", "addSignalStatus",    {{"category","string"}, {"name","string"}, {"epoch","number"}, {"status","string"}}},
        {"ltg", "addSignalValue",     {{"category","string"}, {"name","string"}, {"epoch","number"}, {"value","number"}, {"desc","string"}}},
        {"ltg", "addSignalStartZone", {{"category","string"}, {"name","string"}, {"epoch","number"}, {"startMsg","string"}}},
        {"ltg", "addSignalEndZone",   {{"category","string"}, {"name","string"}, {"epoch","number"}, {"endMsg","string"}}},
        {"ltg", "logInfo",            {{"message","string"}}},
        {"ltg", "logWarning",         {{"message","string"}}},
        {"ltg", "logError",           {{"message","string"}}},
        {"ltg", "logDebug",           {{"message","string"}}},
        {"ltg", "getRowIndex",        {}},
        {"ltg", "getRowCount",        {}},
    };
    return catalog;
}

}  // namespace

void Module::getSignatureInfo(const std::string& aTarget, const std::string& aFunctionName, Ltg::SignatureInfo& aoSignature) {
    if (aFunctionName.empty()) {
        return;
    }
    for (const auto& entry : s_signatureCatalog()) {
        if (aTarget == entry.target && aFunctionName == entry.functionName) {
            aoSignature.label = entry.target[0] != '\0'
                ? std::string(entry.target) + ":" + entry.functionName
                : entry.functionName;
            aoSignature.args.clear();
            aoSignature.args.reserve(entry.args.size());
            for (const auto& catArg : entry.args) {
                Ltg::SignatureArg arg;
                arg.name = catArg.name;
                arg.type = catArg.type;
                aoSignature.args.push_back(std::move(arg));
            }
            return;
        }
    }
    // not in the catalog — leave aoSignature empty, the host won't open the tooltip
}

bool Module::callScriptStart(Ltg::ErrorContainer& vOutErrors) {
    sol::protected_function startFile = (*m_luaPtr)["startFile"];
    if (!startFile.valid()) {
        LogVarLightError("Lua: %s", "the lua function startFile() is missing");
        return false;
    }
    sol::protected_function_result result = startFile();
    if (!result.valid()) {
        sol::error solErr = result;
        Ltg::ScriptingError err;
        parseLuaError(solErr.what(), Ltg::sc_PROJECT_SCRIPT_CHUNK, err);
        vOutErrors.push_back(err);
        LogVarLightError("Lua: error in startFile func call : %s", solErr.what());
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
        sol::error solErr = result;
        Ltg::ScriptingError err;
        parseLuaError(solErr.what(), Ltg::sc_PROJECT_SCRIPT_CHUNK, err);
        vErrors.push_back(err);
        LogVarLightError("Lua: error in parse func call : %s", solErr.what());
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
        sol::error solErr = result;
        Ltg::ScriptingError err;
        parseLuaError(solErr.what(), Ltg::sc_PROJECT_SCRIPT_CHUNK, err);
        vOutErrors.push_back(err);
        LogVarLightError("Lua: error in endFile func call : %s", solErr.what());
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
    // bind `this` to this lua_State through its registry, so the C line hook can recover the
    // Module from the lua_State it is handed
    lua_pushlightuserdata(luaStatePtr, this);
    lua_setfield(luaStatePtr, LUA_REGISTRYINDEX, kDebugModuleRegistryKey);
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
    m_releaseDebugRefs(luaStatePtr);  // free any registry refs left from the last pause
    lua_pushnil(luaStatePtr);  // drop the registry binding (the hook is being removed anyway)
    lua_setfield(luaStatePtr, LUA_REGISTRYINDEX, kDebugModuleRegistryKey);
    m_luaPtr->safe_script("if jit and jit.on then jit.on() end", sol::script_pass_on_error);
    m_debugHostPtr = nullptr;
    m_debugEnabled = false;
}

void Module::requestPause() {
    m_pauseRequested = true;
}

void Module::requestStop() {
    m_stopRequested = true;
}

void Module::sLuaHook(lua_State* apLua, lua_Debug* apDebug) {
    // recover the Module bound to this lua_State (set in enableDebug) from the state's registry
    lua_getfield(apLua, LUA_REGISTRYINDEX, kDebugModuleRegistryKey);
    Module* self = static_cast<Module*>(lua_touserdata(apLua, -1));
    lua_pop(apLua, 1);
    if (self != nullptr) {
        self->m_onHook(apLua, apDebug);
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
    Ltg::DebugAction action = m_debugHostPtr->onPause(state);  // BLOCKS until the first action
    // drain expand/eval requests until the host hands back a control command (Continue/Step/Stop).
    while (action.kind == Ltg::DebugAction::Kind::Expand || action.kind == Ltg::DebugAction::Kind::Eval) {
        if (action.kind == Ltg::DebugAction::Kind::Expand) {
            const std::vector<Ltg::DebugVar> children = m_expandRef(apLua, action.expandRef);
            m_debugHostPtr->publishExpansion(action.expandRef, children);
        } else {  // Eval — watch expression evaluated in the innermost frame's scope
            const Ltg::EvalResult result = m_evalExpression(apLua, apDebug, action.evalExpression);
            m_debugHostPtr->publishEvalResult(action.evalId, result);
        }
        action = m_debugHostPtr->waitAction();  // BLOCKS until the next action
    }
    m_releaseDebugRefs(apLua);
    m_applyCommand(apLua, action.command);
}

bool Module::m_shouldBreak(lua_State* apLua, int32_t aLine) {
    if (m_pauseRequested) {
        return true;
    }
    if (m_debugHostPtr != nullptr && m_debugHostPtr->isBreakpoint(aLine)) {
        return true;  // live query: add/remove during the session is honoured immediately
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

    // each call-stack frame carries its own locals and upvalues (roots only; tables are
    // expanded later on demand through their ref). innermost frame first, capped for safety.
    lua_Debug frameInfo;
    for (int32_t level = 0; level < 64 && lua_getstack(apLua, level, &frameInfo) != 0; ++level) {
        lua_getinfo(apLua, "nSlf", &frameInfo);  // n,S,l + push the frame function (f)
        const int32_t functionIndex = lua_gettop(apLua);

        Ltg::DebugFrame frame;
        frame.function = (frameInfo.name != nullptr) ? frameInfo.name : "";
        frame.source = frameInfo.short_src;
        frame.line = static_cast<int32_t>(frameInfo.currentline);

        int32_t localIndex = 1;
        const char* localName = nullptr;
        while ((localName = lua_getlocal(apLua, &frameInfo, localIndex)) != nullptr) {
            frame.locals.push_back(m_makeVar(apLua, -1, localName, "none"));
            lua_pop(apLua, 1);
            ++localIndex;
        }

        int32_t upvalueIndex = 1;
        const char* upvalueName = nullptr;
        while ((upvalueName = lua_getupvalue(apLua, functionIndex, upvalueIndex)) != nullptr) {
            if (upvalueName[0] != '\0') {
                frame.upvalues.push_back(m_makeVar(apLua, -1, upvalueName, "none"));
            }
            lua_pop(apLua, 1);  // pop the upvalue
            ++upvalueIndex;
        }

        lua_pop(apLua, 1);  // pop the frame function pushed by "f"
        state.callStack.push_back(frame);
    }

    // top-level globals (no filter; each table is expandable on demand)
    lua_pushvalue(apLua, LUA_GLOBALSINDEX);
    const int32_t globalsIndex = lua_gettop(apLua);
    lua_pushnil(apLua);
    while (lua_next(apLua, globalsIndex) != 0) {
        const std::string keyType = lua_typename(apLua, lua_type(apLua, -2));
        const std::string keyName = luaKeyToString(apLua, -2);
        state.globals.push_back(m_makeVar(apLua, -1, keyName, keyType));
        lua_pop(apLua, 1);  // pop value, keep key for the next lua_next
    }
    lua_pop(apLua, 1);  // pop the globals table

    return state;
}

int32_t Module::m_makeRef(lua_State* apLua, int32_t aIndex) {
    if (lua_type(apLua, aIndex) != LUA_TTABLE) {
        return -1;  // only tables are expandable in v1
    }
    lua_pushvalue(apLua, aIndex);
    const int32_t ref = luaL_ref(apLua, LUA_REGISTRYINDEX);  // pops the pushed copy, stores it
    m_debugRefs.push_back(ref);
    return ref;
}

Ltg::DebugVar Module::m_makeVar(lua_State* apLua, int32_t aIndex, const std::string& aName, const std::string& aKeyType) {
    Ltg::DebugVar var;
    var.name = aName;
    var.keyType = aKeyType;
    var.typeName = lua_typename(apLua, lua_type(apLua, aIndex));
    var.value = luaValueToString(apLua, aIndex);
    var.ref = m_makeRef(apLua, aIndex);  // last: m_makeRef is net-neutral on the stack
    return var;
}

std::vector<Ltg::DebugVar> Module::m_expandRef(lua_State* apLua, int32_t aRef) {
    std::vector<Ltg::DebugVar> children;
    lua_rawgeti(apLua, LUA_REGISTRYINDEX, aRef);  // push the referenced value
    const int32_t tableIndex = lua_gettop(apLua);
    if (lua_type(apLua, tableIndex) == LUA_TTABLE) {
        int32_t childCount = 0;
        lua_pushnil(apLua);
        while (lua_next(apLua, tableIndex) != 0) {
            if (childCount >= kMaxExpandChildren) {
                Ltg::DebugVar more;
                more.name = "...";
                more.keyType = "none";
                more.value = "(truncated)";
                children.push_back(more);
                lua_pop(apLua, 2);  // pop value AND key to stop iterating
                break;
            }
            const std::string keyType = lua_typename(apLua, lua_type(apLua, -2));
            const std::string keyName = luaKeyToString(apLua, -2);
            children.push_back(m_makeVar(apLua, -1, keyName, keyType));
            ++childCount;
            lua_pop(apLua, 1);  // pop value, keep key for the next lua_next
        }
    }
    lua_pop(apLua, 1);  // pop the referenced value
    return children;
}

Ltg::EvalResult Module::m_evalExpression(lua_State* apLua, lua_Debug* apDebug, const std::string& aExpression) {
    // evaluate `aExpression` in the innermost paused frame's scope. compile as `return <expr>` so
    // any Lua expression (identifier, indexing, even a call) yields a single value; then build a
    // sandboxed environment table (globals as fallback via __index, then overlay upvalues, then
    // locals — locals win for shadowing) and setfenv the chunk before pcall'ing it.
    Ltg::EvalResult result;
    const int32_t topBefore = lua_gettop(apLua);

    const std::string chunk = "return " + aExpression;
    if (luaL_loadbuffer(apLua, chunk.c_str(), chunk.size(), "watch") != 0) {
        const char* err = lua_tostring(apLua, -1);
        result.error = err != nullptr ? err : "compile error";
        lua_settop(apLua, topBefore);
        return result;
    }
    const int32_t functionIndex = lua_gettop(apLua);

    // env table with _G as the fallback chain (via __index in a metatable)
    lua_newtable(apLua);
    const int32_t envIndex = lua_gettop(apLua);
    lua_newtable(apLua);                          // metatable
    lua_pushvalue(apLua, LUA_GLOBALSINDEX);       // push _G
    lua_setfield(apLua, -2, "__index");           // metatable.__index = _G
    lua_setmetatable(apLua, envIndex);            // setmetatable(env, metatable) — consumes metatable

    // overlay upvalues of the paused function
    lua_getinfo(apLua, "f", apDebug);             // push the current frame's function
    const int32_t pausedFunctionIndex = lua_gettop(apLua);
    int32_t upvalueIndex = 1;
    while (true) {
        const char* upvalueName = lua_getupvalue(apLua, pausedFunctionIndex, upvalueIndex);
        if (upvalueName == nullptr) {
            break;
        }
        if (upvalueName[0] != '\0') {
            lua_setfield(apLua, envIndex, upvalueName);  // env[name] = value (consumes value)
        } else {
            lua_pop(apLua, 1);
        }
        ++upvalueIndex;
    }
    lua_pop(apLua, 1);  // pop the paused function

    // overlay locals (last → win against upvalues / globals)
    int32_t localIndex = 1;
    while (true) {
        const char* localName = lua_getlocal(apLua, apDebug, localIndex);
        if (localName == nullptr) {
            break;
        }
        if (localName[0] != '(' && localName[0] != '\0') {  // skip internal slots like "(*temporary)"
            lua_setfield(apLua, envIndex, localName);
        } else {
            lua_pop(apLua, 1);
        }
        ++localIndex;
    }

    // set the function's environment to env (consumes env from the stack top), then call
    lua_setfenv(apLua, functionIndex);
    if (lua_pcall(apLua, 0, 1, 0) != 0) {
        const char* err = lua_tostring(apLua, -1);
        result.error = err != nullptr ? err : "runtime error";
        lua_settop(apLua, topBefore);
        return result;
    }

    // success: the single return is at top of the stack
    result.value = luaValueToString(apLua, -1);
    result.typeName = lua_typename(apLua, lua_type(apLua, -1));
    lua_settop(apLua, topBefore);
    return result;
}

void Module::m_releaseDebugRefs(lua_State* apLua) {
    for (const int32_t ref : m_debugRefs) {
        luaL_unref(apLua, LUA_REGISTRYINDEX, ref);
    }
    m_debugRefs.clear();
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
            lua_sethook(apLua, nullptr, 0, 0);  // stop hooking so callScriptEnd won't re-trigger
            luaL_error(apLua, "execution stopped by the debugger");  // longjmp, never returns
            break;
    }
}

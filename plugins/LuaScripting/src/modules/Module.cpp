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
        //
        // It also drives the auto-bp-on-error feature: it fires inside the sol2 C-trampoline
        // BEFORE the Lua stack unwinds, so when the host's DebugSettings::AutoBreakpointOnError
        // toggle is on, we can recover the Module from the lua_State registry (key set in
        // enableDebug) and call onPause synchronously with the throwing frame still alive —
        // user can inspect locals/upvalues/call-stack at the point of throw, then continue
        // and the error keeps propagating up to pcall normally.
        m_luaPtr->set_exception_handler(
            [](lua_State* L, sol::optional<const std::exception&> maybe_exception, sol::string_view description) {
                std::string errorMessage;
                if (maybe_exception) {
                    errorMessage = maybe_exception->what();
                } else {
                    errorMessage.assign(description.data(), description.size());
                }
                // recover the Module bound to this lua_State (set in enableDebug; nullptr when
                // debug session isn't armed — i.e. neither the master Debug toggle nor the
                // auto-bp-on-error toggle is on, see ScriptDebugger::shouldArmDebug).
                lua_getfield(L, LUA_REGISTRYINDEX, kDebugModuleRegistryKey);
                Module* self = static_cast<Module*>(lua_touserdata(L, -1));
                lua_pop(L, 1);
                if (self != nullptr && self->m_debugHostPtr != nullptr && self->m_debugHostPtr->shouldPauseOnError()) {
                    // surface the error in the console at the same time as the pause — the catch
                    // block in callScriptExec also logs it (after the user resumes), so this is a
                    // brief duplicate at the moment of the pause, intentional for visibility.
                    LogVarLightError("Lua: %s", errorMessage.c_str());
                    self->m_pauseOnError(L, errorMessage);
                }
                return sol::stack::push(L, errorMessage);
            });

        // Curated Lua stdlib for a log-parsing context. The set below covers everything a parsing
        // script realistically needs while closing off escape routes that don't belong in the
        // sandbox:
        //   - `io` / `ffi` removed: file I/O and raw C call-out are full sandbox escapes — a
        //     malicious or buggy script could trash arbitrary files (io) or corrupt memory (ffi).
        //     The canonical signal sink is `ltg:addSignal*`, not file write.
        //   - `debug` removed: a user `debug.sethook(...)` would silently override our line hook
        //     and break the debugger. We use lua_sethook from C directly, no need to expose it.
        //   - `package` removed: not needed for self-contained parsing scripts; will come back
        //     when multi-script support + a local-lib folder next to the binary lands (the user
        //     explicitly flagged this in advance, do not remove this comment when re-adding it).
        //   - `bit32` removed: Lua 5.2 backport; LuaJIT ships `bit` natively (auto-loaded via
        //     `jit`), so bit32 is redundant.
        //   - `coroutine` removed: exotic in a parsing loop, never seen in practice on logs.
        m_luaPtr->open_libraries(sol::lib::base);    // globals: tostring/tonumber/pairs/pcall/setmetatable/...
        m_luaPtr->open_libraries(sol::lib::string);  // pattern matching, format — core for parsing
        m_luaPtr->open_libraries(sol::lib::math);    // numeric ops
        m_luaPtr->open_libraries(sol::lib::table);   // insert/concat/sort/remove
        m_luaPtr->open_libraries(sol::lib::os);      // date/time/clock — log timestamps
        m_luaPtr->open_libraries(sol::lib::jit);     // we drive jit.off/on around debug sessions

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
        // Shared regex brick — boost::regex wrapped as a Lua usertype. Registered BEFORE
        // LuaDatasModel so that the `regex` factory method's return type is already known to
        // sol2 (order doesn't strictly matter for resolution but keeps the compile order tidy).
        m_luaPtr->new_usertype<LuaRegex>(
            "LtgRegex", sol::no_constructor,  // no direct ctor — created via `ltg:regex(pattern)`
            "test",    &LuaRegex::test,
            "match",   &LuaRegex::match,
            "find",    &LuaRegex::find,
            "gsub",    &LuaRegex::gsub,
            "gmatch",  &LuaRegex::gmatch
        );
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
            "getRowCount", &LuaDatasModel::luaModuleGetRowCount,
            "regex", &LuaDatasModel::luaModuleRegex
        );
        // clang-format on

        (*m_luaPtr)["ltg"] = m_luaDatasModelPtr = LuaDatasModel::create(vDatasModel);

        // Lua-level message handler — registered once via raw Lua API (sol2's `["x"] = func_ptr`
        // can be ambiguous for `int(*)(lua_State*)`; lua_pushcfunction + lua_setglobal is the
        // canonical path for a lua_CFunction). Attached to each protected_function call below so
        // it fires BEFORE the Lua stack unwinds on a pure-Lua error (string.match(nil),
        // nil:method(), bad arg types, ...). The sol2 exception_handler above handles the
        // orthogonal case of C++ exceptions thrown from bindings; both call m_pauseOnError when
        // shouldPauseOnError() is on.
        {
            lua_State* luaStatePtr = m_luaPtr->lua_state();
            lua_pushcfunction(luaStatePtr, &Module::sLuaErrorHandler);
            lua_setglobal(luaStatePtr, "__ltg_error_handler");
        }

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
            LogVarLightError("Lua: %s", "the lua function startFile(filepath) is missing");
            res = false;
        }
        sol::function endFile = (*m_luaPtr)["endFile"];
        if (!endFile.valid()) {
            LogVarLightError("Lua: %s", "the lua function endFile(filepath) is missing");
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
            LogVarLightError("Lua: %s", "the lua function startFile(filepath) is missing");
            res = false;
        }
        sol::function endFile = (*m_luaPtr)["endFile"];
        if (!endFile.valid()) {
            LogVarLightError("Lua: %s", "the lua function endFile(filepath) is missing");
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
    // mirror the main VM's stdlib set so completion shows the same `math.*` / `string.*` / `table.*`
    // / `os.*` keys the user can actually call at runtime. The sandbox exec for setProjectScriptCode
    // has its own narrower allowlist (kSafeStdlib) — that's a separate concern, the completion
    // state itself doesn't run user code.
    m_completionLuaPtr->open_libraries(sol::lib::base);
    m_completionLuaPtr->open_libraries(sol::lib::string);
    m_completionLuaPtr->open_libraries(sol::lib::math);
    m_completionLuaPtr->open_libraries(sol::lib::table);
    m_completionLuaPtr->open_libraries(sol::lib::os);
    m_completionLuaPtr->open_libraries(sol::lib::jit);

    // clang-format off
    // mirror the main VM's usertype registrations so the completion VM exposes the same metatable
    // keys (`ltg:regex`, the regex object's methods, etc.).
    m_completionLuaPtr->new_usertype<LuaRegex>(
        "LtgRegex", sol::no_constructor,
        "test",   &LuaRegex::test,
        "match",  &LuaRegex::match,
        "find",   &LuaRegex::find,
        "gsub",   &LuaRegex::gsub,
        "gmatch", &LuaRegex::gmatch
    );
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
        "getRowCount", &LuaDatasModel::luaModuleGetRowCount,
        "regex", &LuaDatasModel::luaModuleRegex);
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

void Module::setProjectScriptCode(const std::string& aCode) {
    // Push the in-memory project script into the completion state so user globals (top-level
    // `function parse(...)`, `helpers = { ... }`, etc.) surface alongside the stdlib + `ltg`
    // bindings. Strategy: load the chunk, set its env to a fresh sandbox table pre-populated
    // with safe stdlib pointers + a no-op `ltg` stub, pcall it under that env, then steal the
    // sandbox keys back into _G of the completion state. Anything the user code does that the
    // sandbox doesn't allow (io.open, os.exit, print, ...) raises a runtime error inside the
    // pcall and is swallowed — the partial sandbox state is still useful.
    m_ensureCompletionState();
    lua_State* L = m_completionLuaPtr->lua_state();
    const int topBefore = lua_gettop(L);

    // empty code path = explicit clear (e.g. project closed). Wipe + return.
    if (aCode.empty()) {
        for (const auto& key : m_completionUserGlobals) {
            lua_pushnil(L);
            lua_setglobal(L, key.c_str());
        }
        m_completionUserGlobals.clear();
        lua_settop(L, topBefore);
        return;
    }

    // NOTE on wipe order — the wipe of previous globals is DEFERRED until after the new chunk has
    // both parsed and executed successfully (see step 5). Wiping eagerly here would break the
    // user's mid-edit experience: typing inside a function body briefly invalidates the syntax,
    // loadbuffer fails, we'd bail with previous globals already nil'd, and the autocomplete
    // popup typed during that gap finds no targets. The current order keeps `_G` in its last
    // good state across syntactically-invalid edits.

    // 2) build a fresh sandbox table holding only side-effect-free pointers. ltg is replaced
    //    by a metatable-driven stub that returns a no-op closure for any key access, so
    //    `ltg:logInfo("x")` / `ltg:addSignalTag(...)` at top level of the user script don't
    //    spam the host's console / Messaging pane during autocomplete refresh.
    lua_newtable(L);
    const int sandboxIdx = lua_gettop(L);
    // clang-format off
    static const char* const kSafeStdlib[] = {
        "math", "string", "table",
        "tostring", "tonumber", "pairs", "ipairs", "type", "select", "next", "unpack",
        "rawget", "rawset", "rawequal", "setmetatable", "getmetatable",
        "assert", "error", "pcall", "xpcall",
    };
    // clang-format on
    for (const char* const name : kSafeStdlib) {
        lua_getglobal(L, name);
        if (lua_isnil(L, -1)) {
            lua_pop(L, 1);
        } else {
            lua_setfield(L, sandboxIdx, name);
        }
    }
    // ltg stub built via a Lua chunk so we don't need a second C-binding registration:
    // setmetatable({}, { __index = function() return function() end end })
    static const char* const kLtgStubChunk =
        "local noop = function() end "
        "return setmetatable({}, { __index = function(_, _) return noop end })";
    if (luaL_loadstring(L, kLtgStubChunk) == 0 && lua_pcall(L, 0, 1, 0) == 0) {
        lua_setfield(L, sandboxIdx, "ltg");
    } else {
        lua_pop(L, 1);  // pop error message or failed chunk
    }

    // 3) snapshot binding key names so the copy-back step doesn't shadow the real bindings
    //    living in _G (notably: the real `ltg` is the LuaDatasModel userdata used by
    //    getCompletionEntries("ltg"), not the no-op stub we just put in the sandbox).
    std::unordered_set<std::string> bindingKeys;
    lua_pushnil(L);
    while (lua_next(L, sandboxIdx) != 0) {
        if (lua_type(L, -2) == LUA_TSTRING) {
            bindingKeys.insert(std::string(lua_tostring(L, -2)));
        }
        lua_pop(L, 1);  // pop value, keep key for next iter
    }

    // 4) load + setfenv + exec. parse error -> bail (the user is mid-edit, previous globals
    //    have already been wiped above so autocomplete shows only the stdlib until the next
    //    syntactically-valid edit). runtime error -> swallow, keep partial sandbox state.
    if (luaL_loadbuffer(L, aCode.data(), aCode.size(), "<project script completion>") != 0) {
        lua_pop(L, 1);  // pop error message
        lua_settop(L, topBefore);
        return;
    }
    lua_pushvalue(L, sandboxIdx);
    if (lua_setfenv(L, -2) == 0) {
        // chunk isn't a function/userdata/thread — should be impossible after a successful
        // loadbuffer, but bail safely if the runtime ever changes.
        lua_settop(L, topBefore);
        return;
    }
    if (lua_pcall(L, 0, 0, 0) != 0) {
        lua_pop(L, 1);  // pop error message
    }

    // 5) NOW wipe previous user globals (deferred from step 1) and copy the fresh sandbox keys.
    //    pcall succeeded -> the chunk is at least syntactically valid; partial state on a
    //    mid-chunk runtime error is still better than the previous version, so we commit either
    //    way as long as loadbuffer + setfenv + pcall didn't bail us out above.
    for (const auto& key : m_completionUserGlobals) {
        lua_pushnil(L);
        lua_setglobal(L, key.c_str());
    }
    m_completionUserGlobals.clear();

    lua_pushnil(L);
    while (lua_next(L, sandboxIdx) != 0) {
        if (lua_type(L, -2) == LUA_TSTRING) {
            std::string key(lua_tostring(L, -2));
            if (bindingKeys.find(key) == bindingKeys.end()) {
                lua_pushvalue(L, -1);  // dup value
                lua_setglobal(L, key.c_str());
                m_completionUserGlobals.insert(std::move(key));
            }
        }
        lua_pop(L, 1);  // pop value, keep key for next iter
    }

    lua_settop(L, topBefore);
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
    const int targetIdx = lua_gettop(L);
    if (targetType == LUA_TTABLE) {
        // plain table (Lua stdlib namespace, user table, class instance, ...) — iterate the
        // direct keys first (e.g. instance fields `self.prefix`, `self.count`).
        m_iterateLuaTable(L, targetIdx, aoEntries);
        // then hop one level through the metatable's __index to surface inherited methods.
        // This is the standard Lua OOP idiom: `Logger.__index = Logger` + `setmetatable(self, Logger)`
        // means `log:info()` resolves to `Logger.info`. Without this hop, `log:` autocomplete
        // would only see direct instance fields and miss every method on the class table.
        if (lua_getmetatable(L, targetIdx) != 0) {
            lua_getfield(L, -1, "__index");
            if (lua_type(L, -1) == LUA_TTABLE) {
                std::vector<Ltg::CompletionEntry> hopEntries;
                m_iterateLuaTable(L, lua_gettop(L), hopEntries);
                // dedup against direct fields — closer scopes win (a shadowing field on the
                // instance should hide the metatable's same-named method in the popup).
                for (auto& entry : hopEntries) {
                    bool present = false;
                    for (const auto& existing : aoEntries) {
                        if (existing.name == entry.name) {
                            present = true;
                            break;
                        }
                    }
                    if (!present) {
                        aoEntries.push_back(std::move(entry));
                    }
                }
            }
            lua_pop(L, 2);  // __index + metatable
        }
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
    // clang-format off
    static const std::vector<SignatureEntry> catalog = {
        // -------- ltg: usertype methods (cf. new_usertype<LuaDatasModel> in Module::load) --------
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
        {"ltg", "regex",              {{"pattern","string"}}},  // returns a LtgRegex usertype

        // -------- LtgRegex methods (the object returned by `ltg:regex(pattern)`) --------
        // boost::regex semantics, with Lua-style 1-based positions for `find` and the same
        // (string, count) shape as string.gsub for `gsub`. `match` returns capture values as
        // multiple results (or nil); `gmatch` returns a Lua iterator.
        {"LtgRegex", "test",   {{"input","string"}}},
        {"LtgRegex", "match",  {{"input","string"}}},
        {"LtgRegex", "find",   {{"input","string"}}},
        {"LtgRegex", "gsub",   {{"input","string"}, {"replacement","string"}}},
        {"LtgRegex", "gmatch", {{"input","string"}}},

        // -------- math.* (LuaJIT 5.1 stdlib) --------
        // Overloaded variants (max/min/random with varying arity) are listed with their canonical
        // form; the host shows one signature line, no overload picker yet.
        {"math", "abs",        {{"x","number"}}},
        {"math", "ceil",       {{"x","number"}}},
        {"math", "floor",      {{"x","number"}}},
        {"math", "fmod",       {{"x","number"}, {"y","number"}}},
        {"math", "modf",       {{"x","number"}}},
        {"math", "max",        {{"x","number"}, {"...","number"}}},
        {"math", "min",        {{"x","number"}, {"...","number"}}},
        {"math", "exp",        {{"x","number"}}},
        {"math", "log",        {{"x","number"}}},
        {"math", "log10",      {{"x","number"}}},
        {"math", "pow",        {{"x","number"}, {"y","number"}}},
        {"math", "sqrt",       {{"x","number"}}},
        {"math", "sin",        {{"x","number"}}},
        {"math", "cos",        {{"x","number"}}},
        {"math", "tan",        {{"x","number"}}},
        {"math", "asin",       {{"x","number"}}},
        {"math", "acos",       {{"x","number"}}},
        {"math", "atan",       {{"x","number"}}},
        {"math", "atan2",      {{"y","number"}, {"x","number"}}},
        {"math", "sinh",       {{"x","number"}}},
        {"math", "cosh",       {{"x","number"}}},
        {"math", "tanh",       {{"x","number"}}},
        {"math", "deg",        {{"x","number"}}},
        {"math", "rad",        {{"x","number"}}},
        {"math", "frexp",      {{"x","number"}}},
        {"math", "ldexp",      {{"x","number"}, {"e","number"}}},
        {"math", "random",     {{"m","number?"}, {"n","number?"}}},
        {"math", "randomseed", {{"seed","number"}}},

        // -------- string.* (LuaJIT 5.1 stdlib) --------
        // All also callable as method on a string via metatable (`s:match(...)`); the signature
        // catalog matches the `string.fn(s, ...)` namespace form — the trigger doesn't try to
        // resolve `s:match(...)` to the string lib (would need type inference).
        {"string", "byte",    {{"s","string"}, {"i","number?"}, {"j","number?"}}},
        {"string", "char",    {{"...","number"}}},
        {"string", "find",    {{"s","string"}, {"pattern","string"}, {"init","number?"}, {"plain","boolean?"}}},
        {"string", "format",  {{"fmt","string"}, {"...","any"}}},
        {"string", "gmatch",  {{"s","string"}, {"pattern","string"}}},
        {"string", "gsub",    {{"s","string"}, {"pattern","string"}, {"repl","string|table|function"}, {"max","number?"}}},
        {"string", "len",     {{"s","string"}}},
        {"string", "lower",   {{"s","string"}}},
        {"string", "match",   {{"s","string"}, {"pattern","string"}, {"init","number?"}}},
        {"string", "rep",     {{"s","string"}, {"n","number"}, {"sep","string?"}}},
        {"string", "reverse", {{"s","string"}}},
        {"string", "sub",     {{"s","string"}, {"i","number"}, {"j","number?"}}},
        {"string", "upper",   {{"s","string"}}},
        {"string", "dump",    {{"f","function"}}},

        // -------- table.* (LuaJIT 5.1 stdlib) --------
        // insert is overloaded (insert(t, v) or insert(t, pos, v)); listed with the widest form.
        {"table", "concat", {{"t","table"}, {"sep","string?"}, {"i","number?"}, {"j","number?"}}},
        {"table", "insert", {{"t","table"}, {"pos","number?"}, {"value","any"}}},
        {"table", "remove", {{"t","table"}, {"pos","number?"}}},
        {"table", "sort",   {{"t","table"}, {"comp","function?"}}},
        {"table", "maxn",   {{"t","table"}}},

        // -------- os.* (subset useful for log parsing) --------
        // io.* deliberately omitted — file I/O from a parsing script is a smell; ltg:* is the
        // canonical sink for signals/tags/values.
        {"os", "date",     {{"format","string?"}, {"time","number?"}}},
        {"os", "time",     {{"table","table?"}}},
        {"os", "difftime", {{"t2","number"}, {"t1","number"}}},
        {"os", "clock",    {}},
        {"os", "getenv",   {{"name","string"}}},

        // -------- base lib (globals — target left empty) --------
        // The trigger fires on `funcName(` at top level (no dot/colon prefix). Common scaffolding
        // for parsing scripts: tostring/tonumber/pairs/ipairs/type/pcall/setmetatable.
        {"", "tostring",     {{"v","any"}}},
        {"", "tonumber",     {{"v","any"}, {"base","number?"}}},
        {"", "type",         {{"v","any"}}},
        {"", "select",       {{"n","number|string"}, {"...","any"}}},
        {"", "pairs",        {{"t","table"}}},
        {"", "ipairs",       {{"t","table"}}},
        {"", "next",         {{"t","table"}, {"key","any?"}}},
        {"", "unpack",       {{"list","table"}, {"i","number?"}, {"j","number?"}}},
        {"", "assert",       {{"v","any"}, {"msg","string?"}}},
        {"", "error",        {{"msg","any"}, {"level","number?"}}},
        {"", "pcall",        {{"f","function"}, {"...","any"}}},
        {"", "xpcall",       {{"f","function"}, {"handler","function"}, {"...","any"}}},
        {"", "setmetatable", {{"t","table"}, {"mt","table|nil"}}},
        {"", "getmetatable", {{"t","table"}}},
        {"", "rawget",       {{"t","table"}, {"k","any"}}},
        {"", "rawset",       {{"t","table"}, {"k","any"}, {"v","any"}}},
        {"", "rawequal",     {{"a","any"}, {"b","any"}}},
        {"", "print",        {{"...","any"}}},
    };
    // clang-format on
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

bool Module::callScriptStart(const Ltg::ScriptingDatas& vOutDatas, Ltg::ErrorContainer& vOutErrors) {
    sol::protected_function startFile = (*m_luaPtr)["startFile"];
    if (!startFile.valid()) {
        LogVarLightError("Lua: %s", "the lua function startFile(filepath) is missing");
        return false;
    }
    // attach the Lua-level error handler so pure-Lua errors (string.match(nil), nil:method(), ...)
    // trigger m_pauseOnError before the stack unwinds; the existing valid()-check below still
    // catches the propagated error and logs it after the user resumes.
    startFile.set_error_handler((*m_luaPtr)["__ltg_error_handler"]);
    sol::protected_function_result result = startFile(vOutDatas.filepath);
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
    parse.set_error_handler((*m_luaPtr)["__ltg_error_handler"]);
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

bool Module::callScriptEnd(const Ltg::ScriptingDatas& vOutDatas, Ltg::ErrorContainer& vOutErrors) {
    sol::protected_function endFile = (*m_luaPtr)["endFile"];
    if (!endFile.valid()) {
        LogVarLightError("%s", "the lua function endFile(filepath) is missing");
        return false;
    }
    endFile.set_error_handler((*m_luaPtr)["__ltg_error_handler"]);
    sol::protected_function_result result = endFile(vOutDatas.filepath);
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

int Module::sLuaErrorHandler(lua_State* apLua) {
    // The error message is at stack[1] (lua_pcall convention for the message handler).
    // Coerce to a string conservatively — Lua errors are usually strings but the user can
    // raise() arbitrary values.
    std::string message;
    if (lua_isstring(apLua, 1)) {
        message = lua_tostring(apLua, 1);
    } else {
        const int valueType = lua_type(apLua, 1);
        message = std::string(lua_typename(apLua, valueType)) + ": (non-string error value)";
    }
    // recover Module via the registry (set in enableDebug — nullptr when debug not armed,
    // in which case we just pass the message through unchanged).
    lua_getfield(apLua, LUA_REGISTRYINDEX, kDebugModuleRegistryKey);
    Module* self = static_cast<Module*>(lua_touserdata(apLua, -1));
    lua_pop(apLua, 1);
    if (self != nullptr && self->m_debugHostPtr != nullptr && self->m_debugHostPtr->shouldPauseOnError()) {
        // surface the error in the console at the same time as the pause (the catch in
        // callScriptExec also logs after the user resumes — intentional duplicate for visibility).
        LogVarLightError("Lua: %s", message.c_str());
        self->m_pauseOnError(apLua, message);
    }
    // Push the message back as the error value so pcall returns it as-is to sol2 / callScriptExec.
    lua_pushstring(apLua, message.c_str());
    return 1;
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
    m_runPauseLoop(apLua, std::move(state));
    // Stop command was applied inside m_runPauseLoop (m_stopRequested set, hook removed) but the
    // luaL_error raise was deferred — only the line-hook path is safe to longjmp from. Raise it
    // here so the current Lua execution aborts immediately instead of continuing to the next line.
    if (m_stopRequested) {
        luaL_error(apLua, "execution stopped by the debugger");  // longjmp, never returns
    }
}

void Module::m_pauseOnError(lua_State* apLua, const std::string& aErrorMessage) {
    if (m_debugHostPtr == nullptr) {
        return;
    }
    // Don't pause again if Stop has already been issued — at app shutdown, AbortAndJoinWorker
    // sends a single Stop and waits in Join(). The worker unwinds the current error via the
    // existing pause, but `callScriptEnd` then calls the user's endFile() which may error too;
    // without this guard the error handler would call onPause again and block forever on the
    // condvar (UI thread is stuck in Join, can't issue another Stop). Same applies after a
    // mid-run Stop click on a script that errors during cleanup. m_onHook already has the
    // equivalent guard for the line-hook path.
    if (m_stopRequested) {
        return;
    }
    Ltg::DebugState state = m_buildErrorState(apLua, aErrorMessage);
    m_runPauseLoop(apLua, std::move(state));
}

void Module::m_runPauseLoop(lua_State* apLua, Ltg::DebugState aState) {
    // For Eval requests we need the apDebug of the innermost Lua frame (so lua_getinfo / lua_getlocal
    // can read its function + locals). Find it lazily inside the drain loop: in the line-hook path,
    // level 0 is already a Lua frame; in the exception-handler path, level 0 is the sol2 C trampoline,
    // so we skip C frames and stop at the first Lua/main one.
    Ltg::DebugAction action = m_debugHostPtr->onPause(aState);  // BLOCKS until the first action
    while (action.kind == Ltg::DebugAction::Kind::Expand || action.kind == Ltg::DebugAction::Kind::Eval) {
        if (action.kind == Ltg::DebugAction::Kind::Expand) {
            const std::vector<Ltg::DebugVar> children = m_expandRef(apLua, action.expandRef);
            m_debugHostPtr->publishExpansion(action.expandRef, children);
        } else {  // Eval — watch expression evaluated in the innermost Lua frame's scope
            lua_Debug evalFrame;
            bool foundLuaFrame = false;
            for (int32_t level = 0; level < 32 && lua_getstack(apLua, level, &evalFrame) != 0; ++level) {
                lua_getinfo(apLua, "Sl", &evalFrame);
                // what == "Lua" (Lua fn), "main" (chunk), "C" (C fn), "tail" (tail call).
                if (evalFrame.what != nullptr && (evalFrame.what[0] == 'L' || evalFrame.what[0] == 'm')) {
                    foundLuaFrame = true;
                    break;
                }
            }
            if (foundLuaFrame) {
                const Ltg::EvalResult result = m_evalExpression(apLua, &evalFrame, action.evalExpression);
                m_debugHostPtr->publishEvalResult(action.evalId, result);
            } else {
                Ltg::EvalResult result;
                result.error = "no Lua frame available";
                m_debugHostPtr->publishEvalResult(action.evalId, result);
            }
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
    // Strip Lua's `[string "FOO"]` wrapper so state.sourceFile matches the CodePane sheet ids
    // (the project-script sheet uses `<project script>` directly). Same normalization as
    // m_buildErrorState — without it, CodePane's `sheet.filepathName == state.sourceFile`
    // check fails on every line-hook pause, breaking the caret-sync on Step.
    std::string sourceFile(apDebug->short_src != nullptr ? apDebug->short_src : "");
    if (sourceFile.size() >= 11 && sourceFile.compare(0, 9, "[string \"") == 0) {
        const size_t closeQuote = sourceFile.rfind('"');
        if (closeQuote != std::string::npos && closeQuote > 9) {
            sourceFile = sourceFile.substr(9, closeQuote - 9);
        }
    }
    state.sourceFile = sourceFile;
    m_fillCallStackAndGlobals(apLua, state);
    return state;
}

Ltg::DebugState Module::m_buildErrorState(lua_State* apLua, const std::string& aErrorMessage) {
    Ltg::DebugState state;
    state.logRowIndex = m_currentRowIndex;
    state.errorPause = true;
    state.errorMessage = aErrorMessage;

    // For a C++ exception caught by sol2's trampoline, the message we receive is just `e.what()`
    // (no `[string "..."]:LINE:` prefix yet — Lua wraps it later when the error propagates to
    // pcall). So `parseLuaError` can't extract line info from the message at this point. Instead,
    // walk the Lua stack and use the innermost Lua frame's `currentline` and `short_src` directly
    // — that's the actual throw site, regardless of how the error message was formatted.
    lua_Debug topLua;
    bool foundLuaFrame = false;
    for (int32_t level = 0; level < 32 && lua_getstack(apLua, level, &topLua) != 0; ++level) {
        lua_getinfo(apLua, "Sl", &topLua);
        if (topLua.what != nullptr && (topLua.what[0] == 'L' || topLua.what[0] == 'm')) {
            state.line = static_cast<int32_t>(topLua.currentline);
            // strip Lua's `[string "FOO"]` wrapper so the file matches CodePane's sheet ids
            // (the project-script sheet uses `<project script>` directly, not the wrapped form).
            std::string shortSrc(topLua.short_src != nullptr ? topLua.short_src : "");
            if (shortSrc.size() >= 11 && shortSrc.compare(0, 9, "[string \"") == 0) {
                const size_t closeQuote = shortSrc.rfind('"');
                if (closeQuote != std::string::npos && closeQuote > 9) {
                    shortSrc = shortSrc.substr(9, closeQuote - 9);
                }
            }
            state.sourceFile = shortSrc;
            foundLuaFrame = true;
            break;
        }
    }
    // fallback: if the throw originated from a place we couldn't trace, try parsing the message
    // anyway — some errors arrive pre-wrapped (e.g. re-raised by lua-level pcall + xpcall).
    if (!foundLuaFrame) {
        Ltg::ScriptingError parsed;
        parseLuaError(aErrorMessage, Ltg::sc_PROJECT_SCRIPT_CHUNK, parsed);
        state.sourceFile = parsed.file;
        state.line = static_cast<int32_t>(parsed.line);
    }

    m_fillCallStackAndGlobals(apLua, state);
    return state;
}

void Module::m_fillCallStackAndGlobals(lua_State* apLua, Ltg::DebugState& aoState) {
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
        aoState.callStack.push_back(frame);
    }

    // top-level globals (no filter; each table is expandable on demand)
    lua_pushvalue(apLua, LUA_GLOBALSINDEX);
    const int32_t globalsIndex = lua_gettop(apLua);
    lua_pushnil(apLua);
    while (lua_next(apLua, globalsIndex) != 0) {
        const std::string keyType = lua_typename(apLua, lua_type(apLua, -2));
        const std::string keyName = luaKeyToString(apLua, -2);
        aoState.globals.push_back(m_makeVar(apLua, -1, keyName, keyType));
        lua_pop(apLua, 1);  // pop value, keep key for the next lua_next
    }
    lua_pop(apLua, 1);  // pop the globals table
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
            // luaL_error is NOT raised here — it would longjmp from inside a Lua message handler
            // (the sLuaErrorHandler path) or a sol2 C-trampoline (the exception_handler path),
            // which is undefined behaviour / re-caught as OOM. The caller raises the abort error
            // when it knows it's safe (m_onHook, which is a Lua debug hook — luaL_error there is
            // the standard idiom). The error paths skip the raise: an error is already propagating
            // through pcall, and the Stop button has already set ScriptingEngine::s_working = false
            // which breaks the parse loop in m_run after the current row finishes.
            break;
    }
}

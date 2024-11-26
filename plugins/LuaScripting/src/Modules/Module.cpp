#include "Module.h"
#include <ezlibs/ezFile.hpp>
#include <ezlibs/ezTime.hpp>
#include <ImGuiPack.h>
#include <exception>
#include <chrono>
#include <ctime>

#include <lua.hpp>

#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>

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
            "stringToEpoch", &LuaDatasModel::stringToEpoch,
            "epochToString", &LuaDatasModel::epochToString,
            "addSignalTag", &LuaDatasModel::addSignalTag,
            "addSignalStatus", &LuaDatasModel::addSignalStatus,
            "addSignalValue", &LuaDatasModel::addSignalValue,
            "addSignalStartZone", &LuaDatasModel::addSignalStartZone,
            "addSignalEndZone", &LuaDatasModel::addSignalEndZone
        );
        // clang-format on

        m_luaDatasModelPtr = LuaDatasModel::create(vDatasModel);

        (*m_luaPtr)["ltg"] = m_luaDatasModelPtr;

        return (m_luaPtr != nullptr) && (m_luaDatasModelPtr != nullptr) && (!m_datasModel.expired());
    } catch (std::exception& ex) {
        LogVarError("Fail to init Lua : %s", ex.what());
        return false;
    } catch (...) {
        return false;
    }
}

void Module::unload() {
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

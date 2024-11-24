#include "Module.h"
#include <ImGuiPack.h>
#include <EzLibs/EzFile.hpp>
#include <EzLibs/EzTime.hpp>

#include <chrono>
#include <ctime>

#include <lua.hpp>

///////////////////////////////////////////////////
/// CUSTOM LUA FUNCTIONS //////////////////////////
///////////////////////////////////////////////////

// custom print for output redirection to in app console
// based on luaB_print in lbaselib.c
static int lua_int_print_args(lua_State* L) {
    std::string res;
    int n = lua_gettop(L);
    for (int i = 1; i <= n; i++) {
        size_t l;
        const char* s = lua_tolstring(L, i, &l);
        if (i > 1) /* not the first element? */
        {
            res += '\t';
        }
        res += std::string(s, l);
        lua_settop(L, -(n)-1);
    }
    res += '\n';
    LogVarLightInfo("%s", res.c_str());
    return 0;  // return 0 item
}

// secure string args
static std::string get_lua_secure_string(lua_State* L, int arg_idx) {
    size_t len;
    auto str = lua_tolstring(L, arg_idx, &len);
    if (str && len) {
        return std::string(str, len);
    }

    return "";
}

// SetInfos(string)
static int Lua_void_SetInfos_string(lua_State* L) {
    std::string res = get_lua_secure_string(L, 1);

    if (res.empty()) {
        LogVarLightError("%s", "Lua error : string passed to SetInfos is empty");
    } else {
        //Controller::Instance()->SetInfos(res);
    }

    return 0;  // return 0 item
}

// SetRowBufferName(string)
static int Lua_void_SetRowBufferName_string(lua_State* L) {
    std::string res = get_lua_secure_string(L, 1);

    if (res.empty()) {
        LogVarLightError("%s", "Lua error : string passed to SetBufferName is empty");
    } else {
        //Controller::Instance()->SetRowBufferName(res);
    }

    return 0;  // return 0 item
}

// SetFunctionForEachRow(string)
static int Lua_void_SetFunctionForEachRow_string(lua_State* L) {
    std::string res = get_lua_secure_string(L, 1);

    if (res.empty()) {
        LogVarLightError("%s", "Lua error : string passed to SetFunctionForEachLine is empty");
    } else {
        //Controller::Instance()->SetFunctionForEachRow(res);
    }

    return 0;  // return 0 item
}

// SetFunctionForEndFile(string)
static int Lua_void_SetFunctionForEndFile_string(lua_State* L) {
    std::string res = get_lua_secure_string(L, 1);

    if (res.empty()) {
        LogVarLightError("%s", "Lua error : string passed to SetFunctionForEachLine is empty");
    } else {
        //Controller::Instance()->SetFunctionForEndFile(res);
    }

    return 0;  // return 0 item
}

// int GetRowIndex()
static int Lua_int_GetRowIndex_void(lua_State* L) {
    auto row_index = 0;
    //Controller::Instance()->GetRowIndex();

    lua_pushinteger(L, row_index);

    return 1;  // return 1 item
}

// int GetRowCount()
static int Lua_int_GetRowCount_void(lua_State* L) {
    auto row_count = 0;
    //Controller::Instance()->GetRowCount();

    lua_pushinteger(L, row_count);

    return 1;  // return 1 item
}

// LogInfos(string)
static int Lua_void_LogInfo_string(lua_State* L) {
    std::string res = get_lua_secure_string(L, 1);

    if (res.empty()) {
        LogVarLightError("%s", "Lua code error : the string passed to LogInfo is empty");
    } else {
        LogVarLightInfo("%s", res.c_str());
    }

    return 0;  // return 0 item
}

// LogWarning(string)
static int Lua_void_LogWarning_string(lua_State* L) {
    std::string res = get_lua_secure_string(L, 1);

    if (res.empty()) {
        LogVarLightError("%s", "Lua code error : the string passed to LogWarning is empty");
    } else {
        LogVarLightWarning("%s", res.c_str());
    }

    return 0;  // return 0 item
}

// LogError(string)
static int Lua_void_LogError_string(lua_State* L) {
    std::string res = get_lua_secure_string(L, 1);

    if (res.empty()) {
        LogVarLightError("%s", "Lua code error : the string passed to LogError is empty");
    } else {
        LogVarLightError("%s", res.c_str());
    }

    return 0;  // return 0 item
}

// AddSignalValue(signal_category, signal_name, signal_epoch_time, signal_value)
static int Lua_void_AddSignalValue_category_name_date_value(lua_State* L) {
    // params from stack
    const auto arg_0_category = get_lua_secure_string(L, 1);
    const auto arg_1_name = get_lua_secure_string(L, 2);
    const auto arg_2_date = lua_tonumber(L, 3);
    const auto arg_3_value = lua_tonumber(L, 4);

    if (arg_0_category.empty() || arg_1_name.empty()) {
        LogVarLightError("%s", "Lua code error : the category or/and name passed to AddSignalValue are empty");
    } else {
        //DataBase::Instance()->AddSignalTick((SourceFileID)source_file_id, arg_0_category, arg_1_name, arg_2_date, arg_3_value);
    }

    return 0;  // return 0 item
}

// AddSignalTag(date, r, g, b, a, name, help)
static int Lua_void_AddSignalTag_date_color_name_help(lua_State* L) {
    // params from stack
    const auto arg_1_date = lua_tonumber(L, 1);
    const auto arg_2_color_r = lua_tointeger(L, 2);
    const auto arg_3_color_g = lua_tointeger(L, 3);
    const auto arg_4_color_b = lua_tointeger(L, 4);
    const auto arg_5_color_a = lua_tointeger(L, 5);
    const auto arg_6_name = get_lua_secure_string(L, 6);
    const auto arg_7_help = get_lua_secure_string(L, 7);

    if (arg_6_name.empty()) {
        LogVarLightError("%s", "Lua code error : the string passed to LogValue is empty");
    } else {
        auto color = ImVec4((float)arg_2_color_r, (float)arg_3_color_g, (float)arg_4_color_b, (float)arg_5_color_a);
        //DataBase::Instance()->AddSignalTag(arg_1_date, color, arg_6_name, arg_7_help);
    }

    return 0;  // return 0 item
}

// AddSignalStartZone(signal_category, signal_name, signal_epoch_time, signal_string)
static int Lua_void_AddSignalStartZone_category_name_date_string(lua_State* L) {
    // params from stack
    const auto arg_0_category = get_lua_secure_string(L, 1);
    const auto arg_1_name = get_lua_secure_string(L, 2);
    const auto arg_2_date = lua_tonumber(L, 3);
    const auto arg_3_string = get_lua_secure_string(L, 4);

    if (arg_0_category.empty() || arg_1_name.empty()) {
        LogVarLightError("%s", "Lua code error : the category or/and name passed to AddSignalStartZone are empty");
    } else {
        //DataBase::Instance()->AddSignalStatus((SourceFileID)source_file_id, arg_0_category, arg_1_name, arg_2_date, arg_3_string, Controller::sc_START_ZONE);
    }

    return 0;  // return 0 item
}

// AddSignalEndZone(signal_category, signal_name, signal_epoch_time, signal_string)
static int Lua_void_AddSignalEndZone_category_name_date_string(lua_State* L) {
    // params from stack
    const auto arg_0_category = get_lua_secure_string(L, 1);
    const auto arg_1_name = get_lua_secure_string(L, 2);
    const auto arg_2_date = lua_tonumber(L, 3);
    const auto arg_3_string = get_lua_secure_string(L, 4);

    if (arg_0_category.empty() || arg_1_name.empty()) {
        LogVarLightError("%s", "Lua code error : the category or/and name passed to AddSignalEndZone are empty");
    } else {
        //DataBase::Instance()->AddSignalStatus((SourceFileID)source_file_id, arg_0_category, arg_1_name, arg_2_date, arg_3_string, Controller::sc_END_ZONE);
    }

    return 0;  // return 0 item
}

// AddSignalStatus(signal_category, signal_name, signal_epoch_time, signal_string)
static int Lua_void_AddSignalStatus_category_name_date_string(lua_State* L) {
    // params from stack
    const auto arg_0_category = get_lua_secure_string(L, 1);
    const auto arg_1_name = get_lua_secure_string(L, 2);
    const auto arg_2_date = lua_tonumber(L, 3);
    const auto arg_3_string = get_lua_secure_string(L, 4);

    if (arg_0_category.empty() || arg_1_name.empty()) {
        LogVarLightError("%s", "Lua code error : the category or/and name passed to AddSignalStatus are empty");
    } else {
        //DataBase::Instance()->AddSignalStatus((SourceFileID)source_file_id, arg_0_category, arg_1_name, arg_2_date, arg_3_string, "");
    }

    return 0;  // return 0 item
}

// number GetEpochTime(date_time, hour_offset)
// date_time is in format "YYYY-MM-DD HH:MM:SS,MS" or "YYYY-MM-DD HH:MM:SS.MS"
static int Lua_number_GetEpochTimeFrom_date_time_string_offset_int(lua_State* L) {
    // params from stack
    const auto arg_0_datetime = get_lua_secure_string(L, 1);
    const auto arg_1_offset = lua_tonumber(L, 2);
    if (!arg_0_datetime.empty()) {
        std::tm date_heure = {};
        double millisecondes = 0;

        std::stringstream ss(arg_0_datetime);
        ss >> std::get_time(&date_heure, "%Y-%m-%d %H:%M:%S");
        if (ss.peek() == ',' || ss.peek() == '.') {
            ss.ignore();
            ss >> millisecondes;
        }

        // temporaire
        date_heure.tm_hour += (int)arg_1_offset;

        std::time_t temps_epoch = std::mktime(&date_heure);
        double time_number = temps_epoch + millisecondes / 1000;

        lua_pushnumber(L, time_number);

        return 1;  // return 1 item
    }

    return 0;  // return 0 item
}

static lua_State* CreateLuaState() {
    auto lua_state_ptr = luaL_newstate();
    if (lua_state_ptr) {
        luaJIT_setmode(lua_state_ptr, 0, LUAJIT_MODE_ENGINE | LUAJIT_MODE_ON);

        luaL_openlibs(lua_state_ptr);  // lua access to basic libraries

        // register custom functions
        lua_register(lua_state_ptr, "print", lua_int_print_args);
        lua_register(lua_state_ptr, "LogInfo", Lua_void_LogInfo_string);
        lua_register(lua_state_ptr, "LogWarning", Lua_void_LogWarning_string);
        lua_register(lua_state_ptr, "LogError", Lua_void_LogError_string);
        lua_register(lua_state_ptr, "SetInfos", Lua_void_SetInfos_string);
        lua_register(lua_state_ptr, "SetRowBufferName", Lua_void_SetRowBufferName_string);
        lua_register(lua_state_ptr, "SetFunctionForEachRow", Lua_void_SetFunctionForEachRow_string);
        lua_register(lua_state_ptr, "SetFunctionForEndFile", Lua_void_SetFunctionForEndFile_string);
        lua_register(lua_state_ptr, "GetRowIndex", Lua_int_GetRowIndex_void);
        lua_register(lua_state_ptr, "GetRowCount", Lua_int_GetRowCount_void);
        lua_register(lua_state_ptr, "AddSignalValue", Lua_void_AddSignalValue_category_name_date_value);
        lua_register(lua_state_ptr, "AddSignalStartZone", Lua_void_AddSignalStartZone_category_name_date_string);
        lua_register(lua_state_ptr, "AddSignalEndZone", Lua_void_AddSignalEndZone_category_name_date_string);
        lua_register(lua_state_ptr, "GetEpochTime", Lua_number_GetEpochTimeFrom_date_time_string_offset_int);
        lua_register(lua_state_ptr, "AddSignalTag", Lua_void_AddSignalTag_date_color_name_help);
        lua_register(lua_state_ptr, "AddSignalStatus", Lua_void_AddSignalStatus_category_name_date_string);
    }

    return lua_state_ptr;
}

static void DestroyLuaState(lua_State* vlua_State_ptr) {
    lua_close(vlua_State_ptr);
    vlua_State_ptr = nullptr;
}

///////////////////////////////////////////////////
///////////////////////////////////////////////////
///////////////////////////////////////////////////

/*
void Controller::sSetLuaBufferVarContent(lua_State* vLuaState, const std::string& vVarName, const std::string& vContent) {
    if (!vVarName.empty() && !vContent.empty()) {
        auto _cont = vContent;
        if (_cont.find('\"') != std::string::npos) {
            ez::str::replaceString(_cont, "\"", "\\\"");
        }
        const string str = vVarName + " = \"" + _cont + "\"";
        luaL_dostring(vLuaState, str.c_str());
    }
}
*/

Ltg::ScriptingModulePtr Module::create(const SettingsWeak& vSettings) {
    assert(!vSettings.expired());
    auto res = std::make_shared<Module>();
    res->m_Settings = vSettings;
    if (!res->init()) {
        res.reset();
    }
    return res;
}

bool Module::init(Ltg::PluginBridge* vBridgePtr) {
    return true;
}

void Module::unit() {
}

/*bool Module::m_execScriptCode(const std::string& vCode, std::string& vErrors) {
    if (luaL_dostring(m_LuaStatePtr, vCode.c_str()) != LUA_OK) {
        vErrors = get_lua_secure_string(m_LuaStatePtr, -1);

        LogVarLightError("%s", vErrors.c_str());

        return false;
    }

    return true;
}*/

bool Module::load() {
    m_LuaStatePtr = CreateLuaState();
    return m_LuaStatePtr != nullptr;
}

void Module::unload() {
    DestroyLuaState(m_LuaStatePtr);
}

bool Module::compileScript(const Ltg::ScriptFilePathName& vFilePathName, Ltg::ErrorContainer& vOutErrors) {
    return true;
}
bool Module::callScriptInit(Ltg::ErrorContainer& vOutErrors) {
    return true;
}
bool Module::callScriptStart(Ltg::ErrorContainer& vOutErrors) {
    return true;
}
bool Module::callScriptExec(const Ltg::ScriptingDatas& vOutDatas, Ltg::ErrorContainer& vErrors) {
    return true;
}
bool Module::callScriptEnd(Ltg::ErrorContainer& vOutErrors) {
    return true;
}

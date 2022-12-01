#include "LuaEngine.h"

#include <sstream>
#include <fstream>
#include <iostream>
#include <functional>

#include <ctools/Logger.h>
#include <ctools/FileHelper.h>
#include <Helper/Messaging.h>
#include <Engine/Log/LogEngine.h>

extern "C" 
{
#include <lua/src/lua.h>
#include <lua/src/lualib.h>
#include <lua/src/lauxlib.h>
}

///////////////////////////////////////////////////
/// CUSTOM LUA FUNCTIONS //////////////////////////
///////////////////////////////////////////////////

// custom print for output redirection
// based on luaB_print in lbaselib.c
static int lua_int_print_args(lua_State* L) 
{
    std::string res;

    int n = lua_gettop(L);
    for (int i = 1; i <= n; i++) 
    {
        size_t l;
        const char* s = luaL_tolstring(L, i, &l);  /* convert it to string */
        if (i > 1)  /* not the first element? */
        {
            res += '\t';
            //fwrite(("\t"), sizeof(char), (1), stdout); /* add a tab before it */
        }
        res += std::string(s, l);
        //fwrite((s), sizeof(char), (l), stdout); /* print it */
        lua_settop(L, -(n)-1);  /* pop result */
    }
    res += '\n';
    //fwrite(("\n"), sizeof(char), (1), stdout);
    LogVarLightInfo("%s", res.c_str());
   // fflush(stdout);
    return 0;
}


// Test function
static int Lua_void_SetInfos_string(lua_State* L)
{
    std::string res;

    // first param from stack
    size_t len;
    auto str = lua_tolstring(L, 1, &len);
    if (str && len)
    {
        res = std::string(str, len);
    }

    if (res.empty())
    {
        LogVarLightError("Lua error : string passed to SetInfos is empty");
    }
    else
    {
        LuaEngine::Instance()->SetInfos(res);
    }

    return 0;
}

static int Lua_void_SetBufferNameForCurrentLine_string(lua_State* L)
{
    std::string res;

    // first param from stack
    size_t len;
    auto str = lua_tolstring(L, 1, &len);
    if (str && len)
    {
        res = std::string(str, len);
    }

    if (res.empty())
    {
        LogVarLightError("Lua error : string passed to SetBufferForCurrentLine is empty");
    }
    else
    {
        LuaEngine::Instance()->SetBufferNameForCurrentLine(res);
    }

    return 0;
}

static int Lua_void_SetBufferNameForLastLine_string(lua_State* L)
{
    std::string res;

    // first param from stack
    size_t len;
    auto str = lua_tolstring(L, 1, &len);
    if (str && len)
    {
        res = std::string(str, len);
    }

    if (res.empty())
    {
        LogVarLightError("Lua error : string passed to SetBufferForLastLine is empty");
    }
    else
    {
        LuaEngine::Instance()->SetBufferNameForLastLine(res);
    }

    return 0;
}

static int Lua_void_SetFunctionForEachLine_string(lua_State* L)
{
    std::string res;

    // first param from stack
    size_t len;
    auto str = lua_tolstring(L, 1, &len);
    if (str && len)
    {
        res = std::string(str, len);
    }

    if (res.empty())
    {
        LogVarLightError("Lua error : string passed to SetFunctionForEachLine is empty");
    }
    else
    {
        LuaEngine::Instance()->SetFunctionForEachLine(res);
    }

    return 0;
}

static int Lua_void_SetFunctionForEndFile_string(lua_State* L)
{
    std::string res;

    // first param from stack
    size_t len;
    auto str = lua_tolstring(L, 1, &len);
    if (str && len)
    {
        res = std::string(str, len);
    }

    if (res.empty())
    {
        LogVarLightError("Lua error : string passed to SetFunctionForEachLine is empty");
    }
    else
    {
        LuaEngine::Instance()->SetFunctionForEndFile(res);
    }

    return 0;
}

static int Lua_int_GetCurrentRowIndex_void(lua_State* L)
{
    auto row_index = LuaEngine::Instance()->GetCurrentRowIndex();

    lua_pushinteger(L, row_index);

    return 1;
}

static int Lua_void_LogInfo_string(lua_State* L)
{
    const auto arg_0_string = std::string(lua_tostring(L, 1)); // first param from stack

    if (arg_0_string.empty())
    {
        LogVarLightError("Lua code error : the string passed to LogValue is empty");
    }

    LogVarLightInfo("%s", arg_0_string.c_str());

    return 0;
}

static int Lua_void_LogWarning_string(lua_State* L)
{
    const auto arg_0_string = std::string(lua_tostring(L, 1)); // first param from stack

    if (arg_0_string.empty())
    {
        LogVarLightError("Lua code error : the string passed to LogValue is empty");
    }

    LogVarLightWarning("%s", arg_0_string.c_str());

    return 0;
}

static int Lua_void_LogError_string(lua_State* L)
{
    const auto arg_0_string = std::string(lua_tostring(L, 1)); // first param from stack

    if (arg_0_string.empty())
    {
        LogVarLightError("Lua code error : the string passed to LogValue is empty");
    }

    LogVarLightError("%s", arg_0_string.c_str());

    return 0;
}

static int Lua_void_AddSignalValue_category_name_date_value(lua_State* L)
{
    // params from stack
    const auto arg_0_category = std::string(lua_tostring(L, 1));
    const auto arg_1_name = std::string(lua_tostring(L, 2));
    const auto arg_2_date = lua_tonumber(L, 3);
    const auto arg_2_value = lua_tonumber(L, 4);

    if (arg_0_category.empty() || arg_1_name.empty())
    {
        LogVarLightError("Lua code error : the string passed to LogValue is empty");
    }
    else
    {
        LogEngine::Instance()->AddSignalValue(arg_0_category, arg_1_name, arg_2_date, arg_2_value);
    }

    return 0;
}

///////////////////////////////////////////////////
/// INIT/UNIT /////////////////////////////////////
///////////////////////////////////////////////////

bool LuaEngine::Init()
{
    m_LuaState = luaL_newstate();
    if (m_LuaState)
    {
        luaL_openlibs(m_LuaState); // lua access to basic libraries

        // register custom functions
        lua_register(m_LuaState, "print", lua_int_print_args);
        lua_register(m_LuaState, "LogInfo", Lua_void_LogInfo_string);
        lua_register(m_LuaState, "LogWarning", Lua_void_LogWarning_string);
        lua_register(m_LuaState, "LogError", Lua_void_LogError_string);
        lua_register(m_LuaState, "SetInfos", Lua_void_SetInfos_string);
        lua_register(m_LuaState, "SetBufferNameForCurrentLine", Lua_void_SetBufferNameForCurrentLine_string);
        lua_register(m_LuaState, "SetBufferNameForLastLine", Lua_void_SetBufferNameForLastLine_string);
        lua_register(m_LuaState, "SetFunctionForEachLine", Lua_void_SetFunctionForEachLine_string);
        lua_register(m_LuaState, "SetFunctionForEndFile", Lua_void_SetFunctionForEndFile_string);
        lua_register(m_LuaState, "GetCurrentRowIndex", Lua_int_GetCurrentRowIndex_void);
        lua_register(m_LuaState, "AddSignalValue", Lua_void_AddSignalValue_category_name_date_value);

        return true;
    }

	return false;
}

void LuaEngine::Unit()
{

}

bool LuaEngine::ExecScriptOnFile()
{
    if (!m_LuaFilePathName.empty() && !m_LogFilePathName.empty())
    {
        if (FileHelper::Instance()->IsFileExist(m_LogFilePathName))
        {
            std::ifstream file_to_read(m_LogFilePathName);
            if (file_to_read.is_open())
            {
                if (FileHelper::Instance()->IsFileExist(m_LuaFilePathName))
                {
                    try
                    {
                        LogEngine::Instance()->Clear();

                        // interpret lua script
                        if (luaL_dofile(m_LuaState, m_LuaFilePathName.c_str()) != LUA_OK)
                        {
                            LogVarLightError("%s", lua_tostring(m_LuaState, -1));
                        }
                        else
                        {
                            // call function Init
                            if (lua_getglobal(m_LuaState, "Init") == LUA_TFUNCTION)
                            {
                                lua_pcall(m_LuaState, 0, 0, 0);
                            }

                            m_Lua_Current_Row_Line = 0U;
                            while (std::getline(file_to_read, m_Lua_Current_Buffer_Line))
                            {
                                // set current buffer line
                                if (!m_Lua_Last_Buffer_Line_Var_Name.empty())
                                {
                                    Set_Lua_BufferVar_Content(m_Lua_Last_Buffer_Line_Var_Name, m_Lua_Last_Buffer_Line);
                                }

                                // set last buffer line
                                if (!m_Lua_Current_Buffer_Line_Var_Name.empty())
                                {
                                    Set_Lua_BufferVar_Content(m_Lua_Current_Buffer_Line_Var_Name, m_Lua_Current_Buffer_Line);
                                }

                                // call function for each lines
                                if (!m_Lua_Function_To_Call_For_Each_Line.empty())
                                {
                                    if (lua_getglobal(m_LuaState, m_Lua_Function_To_Call_For_Each_Line.c_str()) == LUA_TFUNCTION)
                                    {
                                        lua_pcall(m_LuaState, 0, 0, 0);
                                    }
                                }

                                // current line to last line
                                m_Lua_Last_Buffer_Line = m_Lua_Current_Buffer_Line;

                                // inc row line pos
                                ++m_Lua_Current_Row_Line;
                            }

                            // call function for end of file
                            if (!m_Lua_Function_To_Call_End_File.empty())
                            {
                                if (lua_getglobal(m_LuaState, m_Lua_Function_To_Call_End_File.c_str()) == LUA_TFUNCTION)
                                {
                                    lua_pcall(m_LuaState, 0, 0, 0);
                                }
                            }

                            LogEngine::Instance()->Finalize();
                        }
                    }
                    catch (std::exception& e)
                    {
                        LogVarLightError(e.what());
                    }

                    return true;
                }
            }
        }
    }

    return false;
}

bool LuaEngine::ExecScriptCode(const std::string& vCode, std::string& vErrors)
{
    if (luaL_dostring(m_LuaState, vCode.c_str()) != LUA_OK)
    {
        vErrors = std::string(lua_tostring(m_LuaState, -1));

        LogVarLightError("%s", vErrors.c_str());

        return false;
    }

    return true;
}

void LuaEngine::SetInfos(const std::string& vInfos)
{
    m_Lua_Infos = vInfos;
}

void LuaEngine::SetBufferNameForCurrentLine(const std::string& vName)
{
    m_Lua_Current_Buffer_Line_Var_Name = vName;
}

void LuaEngine::SetBufferNameForLastLine(const std::string& vName)
{
    m_Lua_Last_Buffer_Line_Var_Name = vName;
}

void LuaEngine::SetFunctionForEachLine(const std::string& vName)
{
    m_Lua_Function_To_Call_For_Each_Line = vName;
}

void LuaEngine::SetFunctionForEndFile(const std::string& vName)
{
    m_Lua_Function_To_Call_End_File = vName;
}

int32_t LuaEngine::GetCurrentRowIndex()
{
    return m_Lua_Current_Row_Line;
}

void LuaEngine::SetLuaFilePathName(const std::string& vFilePathName)
{
    m_LuaFilePathName = vFilePathName;
}

std::string LuaEngine::GetLuaFilePathName()
{
    return m_LuaFilePathName;
}

void LuaEngine::SetLogFilePathName(const std::string& vFilePathName)
{
    m_LogFilePathName = vFilePathName;
}

std::string LuaEngine::GetLogFilePathName()
{
    return m_LogFilePathName;
}

///////////////////////////////////////////////////////
//// CONFIGURATION ////////////////////////////////////
///////////////////////////////////////////////////////

std::string LuaEngine::getXml(const std::string& vOffset, const std::string& vUserDatas)
{
    UNUSED(vUserDatas);

    std::string str;

    str += vOffset + "<lua_file>" + m_LuaFilePathName + "</lua_file>\n";
    str += vOffset + "<log_file>" + m_LogFilePathName + "</log_file>\n";

    return str;
}

bool LuaEngine::setFromXml(tinyxml2::XMLElement* vElem, tinyxml2::XMLElement* vParent, const std::string& vUserDatas)
{
    UNUSED(vUserDatas);

    // The value of this child identifies the name of this element
    std::string strName;
    std::string strValue;
    std::string strParentName;

    strName = vElem->Value();
    if (vElem->GetText())
        strValue = vElem->GetText();
    if (vParent != 0)
        strParentName = vParent->Value();

    if (strName == "lua_file")
        m_LuaFilePathName = strValue;
    else if (strName == "log_file")
        m_LogFilePathName = strValue;

    return true;
}

void LuaEngine::Set_Lua_BufferVar_Content(const std::string& vVarName, const std::string& vContent)
{
    if (!vVarName.empty() && !vContent.empty())
    {
        auto _cont = vContent;
        if (_cont.find("\"") != std::string::npos)
        {
            ct::replaceString(_cont, "\"", "\\\"");
        }
        const string str = vVarName + " = \"" + _cont + "\"";
        luaL_dostring(m_LuaState, str.c_str());
    }
}
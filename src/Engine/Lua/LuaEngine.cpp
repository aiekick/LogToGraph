// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/*
Copyright 2022-2022 Stephane Cuillerdier (aka aiekick)

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include "LuaEngine.h"

#include <sstream>
#include <fstream>
#include <iostream>
#include <functional>

#include <ctools/Logger.h>
#include <ctools/FileHelper.h>
#include <Helper/Messaging.h>
#include <Engine/Log/LogEngine.h>
#include <Engine/Graphs/GraphView.h>
#include <Panes/GraphListPane.h>

#include "lua.hpp"

#include <Panes/ToolPane.h>
#include <Panes/GraphGroupPane.h>
#include <Panes/GraphPane.h>
#include <Panes/LogPane.h>
#include <Panes/SignalsHoveredMap.h>
#include <Panes/CodePane.h>

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
        const char* s = lua_tolstring(L, i, &l);
        if (i > 1)  /* not the first element? */
        {
            res += '\t';
        }
        res += std::string(s, l);
        lua_settop(L, -(n)-1);
    }
    res += '\n';
    LogVarLightInfo("%s", res.c_str());
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
        LogEngine::Instance()->AddSignalTick(arg_0_category, arg_1_name, arg_2_date, arg_2_value);
    }

    return 0;
}

///////////////////////////////////////////////////
/// STATIC'S //////////////////////////////////////
///////////////////////////////////////////////////

std::atomic<double> LuaEngine::s_Progress(0.0);
std::atomic<bool> LuaEngine::s_Working(false);
std::atomic<double> LuaEngine::s_GenerationTime(0.0);
std::mutex LuaEngine::s_WorkerThread_Mutex;

///////////////////////////////////////////////////
/// WORKER THREAD /////////////////////////////////
///////////////////////////////////////////////////

void LuaEngine::sLuAnalyse(
    std::atomic<double>& vProgress,
    std::atomic<bool>& vWorking,
    std::atomic<double>& vGenerationTime)
{
    vProgress = 0.0;

    vWorking = true;

    vGenerationTime = 0.0f;

    const int64_t firstTimeMark = std::chrono::duration_cast<std::chrono::milliseconds>
        (std::chrono::system_clock::now().time_since_epoch()).count();
    
    std::string _LuaFilePathName;
    std::string _LogFilePathName;
    std::string _Lua_Current_Buffer_Line_Var_Name;		// current line of buffer
    std::string _Lua_Last_Buffer_Line_Var_Name;			// last line of buffer
    std::string _Lua_Current_Buffer_Line;				// current line of buffer
    std::string _Lua_Last_Buffer_Line;					// last line of buffer
    std::string _Lua_Function_To_Call_For_Each_Line;	// the function to call for each lines
    std::string _Lua_Function_To_Call_End_File;			// the fucntion to call for the end of the file
    size_t _Lua_Current_Row_Line = 0U;					// the current line pos read from file

    LuaEngine::s_WorkerThread_Mutex.lock();
    _LuaFilePathName = LuaEngine::Instance()->GetLuaFilePathName();
    _LogFilePathName = LuaEngine::Instance()->GetLogFilePathName();
    LuaEngine::s_WorkerThread_Mutex.unlock();

    auto _luaState = luaL_newstate();
    if (_luaState)
    {
        luaJIT_setmode(_luaState, 0, LUAJIT_MODE_ENGINE | LUAJIT_MODE_ON);
        
        luaL_openlibs(_luaState); // lua access to basic libraries

        // register custom functions
        lua_register(_luaState, "print", lua_int_print_args);
        lua_register(_luaState, "LogInfo", Lua_void_LogInfo_string);
        lua_register(_luaState, "LogWarning", Lua_void_LogWarning_string);
        lua_register(_luaState, "LogError", Lua_void_LogError_string);
        lua_register(_luaState, "SetInfos", Lua_void_SetInfos_string);
        lua_register(_luaState, "SetBufferNameForCurrentLine", Lua_void_SetBufferNameForCurrentLine_string);
        lua_register(_luaState, "SetBufferNameForLastLine", Lua_void_SetBufferNameForLastLine_string);
        lua_register(_luaState, "SetFunctionForEachLine", Lua_void_SetFunctionForEachLine_string);
        lua_register(_luaState, "SetFunctionForEndFile", Lua_void_SetFunctionForEndFile_string);
        lua_register(_luaState, "GetCurrentRowIndex", Lua_int_GetCurrentRowIndex_void);
        lua_register(_luaState, "AddSignalValue", Lua_void_AddSignalValue_category_name_date_value);

        
        // do code
        if (!_LuaFilePathName.empty() && !_LogFilePathName.empty())
        {
            if (FileHelper::Instance()->IsFileExist(_LogFilePathName))
            {
                auto file_string = FileHelper::Instance()->LoadFileToString(_LogFilePathName);
                if (!file_string.empty())
                {
                    if (FileHelper::Instance()->IsFileExist(_LuaFilePathName))
                    {
                        try
                        {
                            LogEngine::Instance()->Clear();
                            GraphView::Instance()->Clear();

                            // interpret lua script
                            if (luaL_dofile(_luaState, _LuaFilePathName.c_str()) != LUA_OK)
                            {
                                LogVarLightError("%s", lua_tostring(_luaState, -1));
                            }
                            else
                            {
                                // call function Init
                                lua_getglobal(_luaState, "Init");
                                if (lua_isfunction(_luaState, -1))
                                {
                                    lua_pcall(_luaState, 0, 0, 0);

                                    LuaEngine::s_WorkerThread_Mutex.lock();
                                    _Lua_Current_Buffer_Line_Var_Name = LuaEngine::Instance()->GetBufferNameForCurrentLine();
                                    _Lua_Last_Buffer_Line_Var_Name = LuaEngine::Instance()->GetBufferNameForLastLine();
                                    _Lua_Function_To_Call_For_Each_Line = LuaEngine::Instance()->GetFunctionForEachLine();
                                    _Lua_Function_To_Call_End_File = LuaEngine::Instance()->GetFunctionForEndFile();
                                    LuaEngine::s_WorkerThread_Mutex.unlock();
                                }

                                auto file_lines = ct::splitStringToVector(file_string, '\n');
                                auto count_lines = file_lines.size();

                                _Lua_Current_Row_Line = 0U;
                                for (auto _Lua_Current_Buffer_Line : file_lines)
                                {
                                    if (!vWorking)
                                        break;

                                    // time
                                    const int64_t secondTimeMark = std::chrono::duration_cast<std::chrono::milliseconds>
                                        (std::chrono::system_clock::now().time_since_epoch()).count();

                                    vGenerationTime = (double)(secondTimeMark - firstTimeMark) / 1000.0;
                                    vProgress = (double)_Lua_Current_Row_Line / (double)count_lines;

                                    // set current buffer line
                                    if (!_Lua_Last_Buffer_Line_Var_Name.empty())
                                    {
                                        sSetLuaBufferVarContent(_luaState, _Lua_Last_Buffer_Line_Var_Name, _Lua_Last_Buffer_Line);
                                    }

                                    // set last buffer line
                                    if (!_Lua_Current_Buffer_Line_Var_Name.empty())
                                    {
                                        sSetLuaBufferVarContent(_luaState, _Lua_Current_Buffer_Line_Var_Name, _Lua_Current_Buffer_Line);
                                    }

                                    // call function for each lines
                                    if (!_Lua_Function_To_Call_For_Each_Line.empty())
                                    {
                                        lua_getglobal(_luaState, _Lua_Function_To_Call_For_Each_Line.c_str());
                                        if (lua_isfunction(_luaState, -1))
                                        {
                                            lua_pcall(_luaState, 0, 0, 0);
                                        }
                                    }

                                    // current line to last line
                                    _Lua_Last_Buffer_Line = std::move(_Lua_Current_Buffer_Line);

                                    // inc row line pos
                                    ++_Lua_Current_Row_Line;
                                }

                                // call function for end of file
                                if (!_Lua_Function_To_Call_End_File.empty())
                                {
                                    lua_getglobal(_luaState, _Lua_Function_To_Call_End_File.c_str());
                                    if (lua_isfunction(_luaState, -1))
                                    {
                                        lua_pcall(_luaState, 0, 0, 0);
                                    }
                                }

                                LogEngine::Instance()->Finalize();
                            }
                        }
                        catch (std::exception& e)
                        {
                            LogVarLightError(e.what());
                        }
                    }
                }
            }
        }

        lua_close(_luaState);
    }

    vWorking = false;
}

void LuaEngine::sSetLuaBufferVarContent(lua_State* vLuaState, const std::string& vVarName, const std::string& vContent)
{
    if (!vVarName.empty() && !vContent.empty())
    {
        auto _cont = vContent;
        if (_cont.find('\"') != std::string::npos)
        {
            ct::replaceString(_cont, "\"", "\\\"");
        }
        const string str = vVarName + " = \"" + _cont + "\"";
        luaL_dostring(vLuaState, str.c_str());
    }
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
    lua_close(m_LuaState);
}

/*bool LuaEngine::ExecScriptOnFile()
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
                        GraphView::Instance()->Clear();
                        ToolPane::Instance()->Clear();
                        LogPane::Instance()->Clear();

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
                                    sSetLuaBufferVarContent(m_LuaState, m_Lua_Last_Buffer_Line_Var_Name, m_Lua_Last_Buffer_Line);
                                }

                                // set last buffer line
                                if (!m_Lua_Current_Buffer_Line_Var_Name.empty())
                                {
                                    sSetLuaBufferVarContent(m_LuaState, m_Lua_Current_Buffer_Line_Var_Name, m_Lua_Current_Buffer_Line);
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
}*/

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

std::string LuaEngine::GetInfos()
{
    return m_Lua_Infos;
}

void LuaEngine::SetBufferNameForCurrentLine(const std::string& vName)
{
    m_Lua_Current_Buffer_Line_Var_Name = vName;
}

std::string LuaEngine::GetBufferNameForCurrentLine()
{
    return m_Lua_Current_Buffer_Line_Var_Name;
}

void LuaEngine::SetBufferNameForLastLine(const std::string& vName)
{
    m_Lua_Last_Buffer_Line_Var_Name = vName;
}

std::string LuaEngine::GetBufferNameForLastLine()
{
    return m_Lua_Last_Buffer_Line_Var_Name;
}

void LuaEngine::SetFunctionForEachLine(const std::string& vName)
{
    m_Lua_Function_To_Call_For_Each_Line = vName;
}

std::string LuaEngine::GetFunctionForEachLine()
{
    return m_Lua_Function_To_Call_For_Each_Line;
}

void LuaEngine::SetFunctionForEndFile(const std::string& vName)
{
    m_Lua_Function_To_Call_End_File = vName;
}

std::string LuaEngine::GetFunctionForEndFile()
{
    return m_Lua_Function_To_Call_End_File;
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
//// THREAD ///////////////////////////////////////////
///////////////////////////////////////////////////////

void LuaEngine::StartWorkerThread(const bool& vFirstLoad)
{
    if (!StopWorkerThread())
    {
        if (!vFirstLoad)
        {
            LogEngine::Instance()->PrepareForSave();
        }

        LogEngine::Instance()->Clear();
        GraphView::Instance()->Clear();
        ToolPane::Instance()->Clear();
        LogPane::Instance()->Clear();

        LuaEngine::s_Working = true;

        m_WorkerThread =
            std::thread(
                sLuAnalyse,
                std::ref(LuaEngine::s_Progress),
                std::ref(LuaEngine::s_Working),
                std::ref(LuaEngine::s_GenerationTime));
    }
}

bool LuaEngine::StopWorkerThread()
{
    bool res = false;

    res = IsJoinable();
    if (res)
    {
        LuaEngine::s_Working = false;
        Join();
    }

    return res;
}

bool LuaEngine::IsJoinable()
{
    return m_WorkerThread.joinable();
}

void LuaEngine::Join()
{
    m_WorkerThread.join();
}

bool LuaEngine::FinishIfRequired()
{
    if (m_WorkerThread.joinable())
    {
        if (!LuaEngine::s_Working)
        {
            Join();
            LogEngine::Instance()->PrepareAfterLoad();
            return true;
        }
    }
    return false;
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

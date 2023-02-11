// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/*
Copyright 2022-2023 Stephane Cuillerdier (aka aiekick)

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

#include <iostream>
#include <functional>

#include <ctools/Logger.h>
#include <ctools/FileHelper.h>
#include <Helper/Messaging.h>
#include <Engine/Log/LogEngine.h>
#include <Engine/Graphs/GraphView.h>
#include <Panes/GraphListPane.h>
#include <Panes/LogPaneSecondView.h>
#include <lua.hpp>

#include <Engine/DB/DBEngine.h>
#include <Project/ProjectFile.h>

#include <Panes/ToolPane.h>
#include <Panes/LogPane.h>

///////////////////////////////////////////////////
/// STATIC ////////////////////////////////////////
///////////////////////////////////////////////////

static size_t source_file_id = 0U;

///////////////////////////////////////////////////
/// CUSTOM LUA FUNCTIONS //////////////////////////
///////////////////////////////////////////////////

// custom print for output redirection to in app console
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

// SetInfos(string)
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
        LogVarLightError("%s", "Lua error : string passed to SetInfos is empty");
    }
    else
    {
        LuaEngine::Instance()->SetInfos(res);
    }

    return 0;
}

// SetRowBufferName(string)
static int Lua_void_SetRowBufferName_string(lua_State* L)
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
        LogVarLightError("%s", "Lua error : string passed to SetBufferName is empty");
    }
    else
    {
        LuaEngine::Instance()->SetRowBufferName(res);
    }

    return 0;
}

// SetFunctionForEachRow(string)
static int Lua_void_SetFunctionForEachRow_string(lua_State* L)
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
        LogVarLightError("%s", "Lua error : string passed to SetFunctionForEachLine is empty");
    }
    else
    {
        LuaEngine::Instance()->SetFunctionForEachRow(res);
    }

    return 0;
}

// SetFunctionForEndFile(string)
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
        LogVarLightError("%s", "Lua error : string passed to SetFunctionForEachLine is empty");
    }
    else
    {
        LuaEngine::Instance()->SetFunctionForEndFile(res);
    }

    return 0;
}

// int GetRowIndex()
static int Lua_int_GetRowIndex_void(lua_State* L)
{
    auto row_index = LuaEngine::Instance()->GetRowIndex();

    lua_pushinteger(L, row_index);

    return 1;
}

// int GetRowCount()
static int Lua_int_GetRowCount_void(lua_State* L)
{
    auto row_count = LuaEngine::Instance()->GetRowCount();

    lua_pushinteger(L, row_count);

    return 1;
}

// LogInfos(string)
static int Lua_void_LogInfo_string(lua_State* L)
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
        LogVarLightError("%s", "Lua code error : the string passed to LogInfo is empty");
    }
    else
    {
        LogVarLightInfo("%s", res.c_str());
    }

    return 0;
}

// LogWarning(string)
static int Lua_void_LogWarning_string(lua_State* L)
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
        LogVarLightError("%s", "Lua code error : the string passed to LogWarning is empty");
    }
    else
    {
        LogVarLightWarning("%s", res.c_str());
    }

    return 0;
}

// LogError(string)
static int Lua_void_LogError_string(lua_State* L)
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
        LogVarLightError("%s", "Lua code error : the string passed to LogError is empty");
    }
    else
    {
        LogVarLightError("%s", res.c_str());
    }

    return 0;
}

// AddSignalValue(signal_category, signal_name, signal_epoch_time, signal_value)
static int Lua_void_AddSignalValue_category_name_date_value(lua_State* L)
{
    // params from stack
    const auto arg_0_category = std::string(lua_tostring(L, 1));
    const auto arg_1_name = std::string(lua_tostring(L, 2));
    const auto arg_2_date = lua_tonumber(L, 3);
    const auto arg_2_value = lua_tonumber(L, 4);

    if (arg_0_category.empty() || arg_1_name.empty())
    {
        LogVarLightError("%s", "Lua code error : the string passed to LogValue is empty");
    }
    else
    {
        DBEngine::Instance()->AddSignalTick(source_file_id, arg_0_category, arg_1_name, arg_2_date, arg_2_value);
        LuaEngine::Instance()->AddSignalValue(arg_0_category, arg_1_name, arg_2_date, arg_2_value);
    }

    return 0;
}

static lua_State* CreateLuaState()
{
    auto lua_state_ptr = luaL_newstate();
    if (lua_state_ptr)
    {
        luaJIT_setmode(lua_state_ptr, 0, LUAJIT_MODE_ENGINE | LUAJIT_MODE_ON);

        luaL_openlibs(lua_state_ptr); // lua access to basic libraries

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
    }

    return lua_state_ptr;
}

static void DestroyLuaState(lua_State* vlua_State_ptr)
{
    lua_close(vlua_State_ptr);
    vlua_State_ptr = nullptr;
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
    
    std::string luaFilePathName = LuaEngine::Instance()->GetLuaFilePathName();
    std::string logFilePathName = LuaEngine::Instance()->GetLogFilePathName();
    std::string lua_Current_Buffer_Row_Var_Name;		// current line of buffer
    std::string lua_Function_To_Call_For_Each_Row;	    // the function to call for each lines
    std::string lua_Function_To_Call_End_File;			// the fucntion to call for the end of the file
    int32_t lua_Row_Index = 0;					        // the current line pos read from file
    int32_t lua_Row_Count = 0;					        // the current line pos read from file

    auto _luaState = CreateLuaState();
    if (_luaState)
    {
       if (!luaFilePathName.empty() &&
            !logFilePathName.empty() &&
            FileHelper::Instance()->IsFileExist(logFilePathName))
        {
            auto file_string = FileHelper::Instance()->LoadFileToString(logFilePathName);
            if (!file_string.empty())
            {
                if (FileHelper::Instance()->IsFileExist(luaFilePathName))
                {
                    try
                    {
                        LogEngine::Instance()->Clear();
                        GraphView::Instance()->Clear();

                        auto ps = FileHelper::Instance()->ParsePathFileName(ProjectFile::Instance()->m_ProjectFilePathName);
                        auto dbFile = ps.GetFPNE_WithExt("db");
                        DBEngine::Instance()->OpenDBFile(dbFile);
                        
                        source_file_id = DBEngine::Instance()->AddSourceFile(logFilePathName);

                        DBEngine::Instance()->BeginTransaction();
                        
                        // interpret lua script
                        if (luaL_dofile(_luaState, luaFilePathName.c_str()) != LUA_OK)
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

                                lua_Current_Buffer_Row_Var_Name = LuaEngine::Instance()->GetRowBufferName();
                                lua_Function_To_Call_For_Each_Row = LuaEngine::Instance()->GetFunctionForEachRow();
                                lua_Function_To_Call_End_File = LuaEngine::Instance()->GetFunctionForEndFile();
                            }

                            auto file_lines = ct::splitStringToVector(file_string, '\n');
                            lua_Row_Count = (int32_t)file_lines.size();

                            LuaEngine::Instance()->SetRowCount(lua_Row_Count);

                            lua_Row_Index = 0U;
                            for (auto lua_Current_Buffer_Row_Content : file_lines)
                            {
                                if (!vWorking)
                                    break;

                                LuaEngine::Instance()->SetRowIndex(lua_Row_Index);

                                // time
                                const int64_t secondTimeMark = std::chrono::duration_cast<std::chrono::milliseconds>
                                    (std::chrono::system_clock::now().time_since_epoch()).count();

                                vGenerationTime = (double)(secondTimeMark - firstTimeMark) / 1000.0;
                                vProgress = (double)lua_Row_Index / (double)lua_Row_Count;

                                // set current buffer line
                                if (!lua_Current_Buffer_Row_Var_Name.empty())
                                {
                                    sSetLuaBufferVarContent(_luaState, lua_Current_Buffer_Row_Var_Name, lua_Current_Buffer_Row_Content);
                                }

                                // call function for each row
                                if (!lua_Function_To_Call_For_Each_Row.empty())
                                {
                                    lua_getglobal(_luaState, lua_Function_To_Call_For_Each_Row.c_str());
                                    if (lua_isfunction(_luaState, -1))
                                    {
                                        lua_pcall(_luaState, 0, 0, 0);
                                    }
                                }

                                // inc row line pos
                                ++lua_Row_Index;
                            }

                            // call function for end of file
                            if (!lua_Function_To_Call_End_File.empty())
                            {
                                lua_getglobal(_luaState, lua_Function_To_Call_End_File.c_str());
                                if (lua_isfunction(_luaState, -1))
                                {
                                    lua_pcall(_luaState, 0, 0, 0);
                                }
                            }

                            DBEngine::Instance()->EndTransaction();
                            DBEngine::Instance()->CloseDBFile();
                            LogEngine::Instance()->Finalize(); 

                        }
                    }
                    catch (std::exception& e)
                    {
                        LogVarLightError("%s", e.what());
                    }
                }
            }
        }

       DestroyLuaState(_luaState);
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
    m_LuaStatePtr = CreateLuaState();
	return m_LuaStatePtr != nullptr;
}

void LuaEngine::Unit()
{
    DestroyLuaState(m_LuaStatePtr);
}

bool LuaEngine::ExecScriptCode(const std::string& vCode, std::string& vErrors)
{
    if (luaL_dostring(m_LuaStatePtr, vCode.c_str()) != LUA_OK)
    {
        vErrors = std::string(lua_tostring(m_LuaStatePtr, -1));

        LogVarLightError("%s", vErrors.c_str());

        return false;
    }

    return true;
}

void LuaEngine::SetInfos(const std::string& vInfos)
{
    m_Lua_Script_Description = vInfos;
}

std::string LuaEngine::GetInfos()
{
    return m_Lua_Script_Description;
}

void LuaEngine::SetRowBufferName(const std::string& vName)
{
    m_Lua_Row_Buffer_Var_Name = vName;
}

std::string LuaEngine::GetRowBufferName()
{
    return m_Lua_Row_Buffer_Var_Name;
}

void LuaEngine::SetFunctionForEachRow(const std::string& vName)
{
    m_Lua_Function_To_Call_For_Each_Line = vName;
}

std::string LuaEngine::GetFunctionForEachRow()
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

void LuaEngine::SetRowIndex(const int32_t& vRowID)
{
    m_Lua_Row_Index = vRowID;
}

int32_t LuaEngine::GetRowIndex() const
{
    return m_Lua_Row_Index;
}

void LuaEngine::SetRowCount(const int32_t& vRowCount)
{
    m_Lua_Row_Count = vRowCount;
}

int32_t LuaEngine::GetRowCount() const
{
    return m_Lua_Row_Count;
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

void LuaEngine::AddSignalValue(const SignalCategory& vCategory, const SignalName& vName, const SignalEpochTime& vDate, const SignalValue& vValue)
{
    LogEngine::Instance()->AddSignalTick(vCategory, vName, vDate, vValue);
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
            LogPane::Instance()->Clear();
            LogPaneSecondView::Instance()->Clear();
            GraphListPane::Instance()->UpdateDB();
            ToolPane::Instance()->UpdateTree();
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

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

#include "ScriptingEngine.h"

#include <iostream>
#include <functional>

#include <models/log/LogEngine.h>
#include <models/graphs/GraphView.h>
#include <panes/GraphListPane.h>
#include <panes/LogPaneSecondView.h>

#include <models/database/DataBase.h>
#include <Project/ProjectFile.h>

#include <panes/ToolPane.h>
#include <panes/LogPane.h>

#include <ezlibs/ezFile.hpp>

using namespace std::chrono;

///////////////////////////////////////////////////
/// STATIC ////////////////////////////////////////
///////////////////////////////////////////////////

static size_t source_file_id = 0U;
static SourceFileWeak source_file_parent;

///////////////////////////////////////////////////
/// STATIC'S //////////////////////////////////////
///////////////////////////////////////////////////

std::atomic<double> ScriptingEngine::s_Progress(0.0);
std::atomic<bool> ScriptingEngine::s_Working(false);
std::atomic<double> ScriptingEngine::s_GenerationTime(0.0);
std::mutex ScriptingEngine::s_WorkerThread_Mutex;

///////////////////////////////////////////////////
/// WORKER THREAD /////////////////////////////////
///////////////////////////////////////////////////

void ScriptingEngine::m_Run(std::atomic<double>& vProgress, std::atomic<bool>& vWorking, std::atomic<double>& vGenerationTime) {
    vProgress = 0.0;

    vWorking = true;

    vGenerationTime = 0.0f;

    const int64_t firstTimeMark = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();

    s_WorkerThread_Mutex.lock();

    const auto scriptFilePathName = m_ScriptFilePathName;
    const auto sourceFilePathNames = m_SourceFilePathNames;

    s_WorkerThread_Mutex.unlock();

    std::string lua_Current_Buffer_Row_Var_Name;    // current line of buffer
    std::string lua_Function_To_Call_For_Each_Row;  // the function to call for each lines
    std::string lua_Function_To_Call_End_File;      // the fucntion to call for the end of the file
    int32_t lua_Row_Index = 0;                      // the current line pos read from file
    int32_t lua_Row_Count = 0;                      // the current line pos read from file

    if (!scriptFilePathName.empty()) {
        if (ez::file::isFileExist(scriptFilePathName)) {
            LogEngine::Instance()->Clear();
            GraphView::Instance()->Clear();
            DataBase::Instance()->OpenDBFile(ProjectFile::Instance()->m_ProjectFilePathName);
            DataBase::Instance()->ClearDataTables();
            for (const auto& source_file : sourceFilePathNames) {
                if (!source_file.empty() && ez::file::isFileExist(source_file)) {
                    auto file_string = ez::file::loadFileToString(source_file);
                    if (!file_string.empty()) {
                        try {
                            source_file_id = DataBase::Instance()->AddSourceFile(source_file);
                            DataBase::Instance()->BeginTransaction();
                            if (!m_compileScript(scriptFilePathName)) {
                                LogVarLightError("Fail to compile script \"%s\"", scriptFilePathName.c_str());
                            } else {
                                if (m_callScriptInit()) {
                                    auto file_lines = ez::str::splitStringToVector(file_string, '\n');
                                    lua_Row_Count = (int32_t)file_lines.size();
                                    SetRowCount(lua_Row_Count);
                                    lua_Row_Index = 0U;
                                    for (auto lua_Current_Buffer_Row_Content : file_lines) {
                                        if (!vWorking) {
                                            break;
                                        }
                                        SetRowIndex(lua_Row_Index);
                                        const int64_t secondTimeMark = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
                                        vGenerationTime = (double)(secondTimeMark - firstTimeMark) / 1000.0;
                                        vProgress = (double)lua_Row_Index / (double)lua_Row_Count;
                                        m_callScriptExec(lua_Current_Buffer_Row_Content);
                                        ++lua_Row_Index;
                                    }
                                    m_callScriptEnd();
                                }
                            }
                            DataBase::Instance()->CommitTransaction();
                        } catch (std::exception& e) {
                            LogVarLightError("%s", e.what());
                            DataBase::Instance()->RollbackTransaction();
                        }
                    }
                }
            }

            // retrieve datas from database
            LogEngine::Instance()->Finalize();
            DataBase::Instance()->CloseDBFile();
        }
    }

    vWorking = false;
}

bool ScriptingEngine::m_compileScript(const std::string& vFilePathName) {
    return true;
}

bool ScriptingEngine::m_callScriptInit() {
    return true;
}

bool ScriptingEngine::m_callScriptExec(const std::string& vRow) {
    return true;
}

bool ScriptingEngine::m_callScriptEnd() {
    return true;
}

///////////////////////////////////////////////////
/// INIT/UNIT /////////////////////////////////////
///////////////////////////////////////////////////

void ScriptingEngine::Clear() {
    m_ScriptFilePathName.clear();
    m_SourceFilePathNames.clear();
}

bool ScriptingEngine::Init() {
    return true;
}

void ScriptingEngine::Unit() {
}

void ScriptingEngine::SetInfos(const std::string& vInfos) {
    std::lock_guard<std::mutex> guard(s_WorkerThread_Mutex);
    m_ScriptDescription = vInfos;
}

std::string ScriptingEngine::GetInfos() const {
    std::lock_guard<std::mutex> guard(s_WorkerThread_Mutex);
    return m_ScriptDescription;
}

void ScriptingEngine::SetFunctionForEachRow(const std::string& vName) {
    std::lock_guard<std::mutex> guard(s_WorkerThread_Mutex);
    m_ScriptFuncToCallForEachRow = vName;
}

std::string ScriptingEngine::GetFunctionForEachRow() const {
    std::lock_guard<std::mutex> guard(s_WorkerThread_Mutex);
    return m_ScriptFuncToCallForEachRow;
}

void ScriptingEngine::SetFunctionForEndFile(const std::string& vName) {
    std::lock_guard<std::mutex> guard(s_WorkerThread_Mutex);
    m_ScriptFuncToCallEndFile = vName;
}

std::string ScriptingEngine::GetFunctionForEndFile() const {
    std::lock_guard<std::mutex> guard(s_WorkerThread_Mutex);
    return m_ScriptFuncToCallEndFile;
}

void ScriptingEngine::SetRowIndex(const int32_t& vRowID) {
    std::lock_guard<std::mutex> guard(s_WorkerThread_Mutex);
    m_ScriptRowIndex = vRowID;
}

int32_t ScriptingEngine::GetRowIndex() const {
    std::lock_guard<std::mutex> guard(s_WorkerThread_Mutex);
    return m_ScriptRowIndex;
}

void ScriptingEngine::SetRowCount(const int32_t& vRowCount) {
    std::lock_guard<std::mutex> guard(s_WorkerThread_Mutex);
    m_ScriptRowCount = vRowCount;
}

int32_t ScriptingEngine::GetRowCount() const {
    std::lock_guard<std::mutex> guard(s_WorkerThread_Mutex);
    return m_ScriptRowCount;
}

void ScriptingEngine::SetScriptFilePathName(const SourceFilePathName& vFilePathName) {
    m_ScriptFilePathName = vFilePathName;
}

void ScriptingEngine::AddSourceFilePathName(const SourceFilePathName& vFilePathName) {
    m_SourceFilePathNames.push_back(vFilePathName);
}

void ScriptingEngine::AddSignalValue(const SignalCategory& vCategory, const SignalName& vName, const SignalEpochTime& vDate, const SignalValue& vValue) {
    LogEngine::Instance()->AddSignalTick(source_file_parent, vCategory, vName, vDate, vValue);
}

void ScriptingEngine::AddSignalStatus(const SignalCategory& vCategory,
                                      const SignalName& vName,
                                      const SignalEpochTime& vDate,
                                      const SignalString& vString,
                                      const SignalStatus& vStatus) {
    LogEngine::Instance()->AddSignalStatus(source_file_parent, vCategory, vName, vDate, vString, vStatus);
}

///////////////////////////////////////////////////////
//// THREAD ///////////////////////////////////////////
///////////////////////////////////////////////////////

void ScriptingEngine::StartWorkerThread(const bool& vFirstLoad) {
    if (!StopWorkerThread()) {
        if (!vFirstLoad) {
            LogEngine::Instance()->PrepareForSave();
        }
        LogEngine::Instance()->Clear();
        GraphView::Instance()->Clear();
        ToolPane::Instance()->Clear();
        LogPane::Instance()->Clear();
        ScriptingEngine::s_Working = true;
        m_WorkerThread = std::thread(  //
            &ScriptingEngine::m_Run,
            this,
            std::ref(ScriptingEngine::s_Progress),
            std::ref(ScriptingEngine::s_Working),
            std::ref(ScriptingEngine::s_GenerationTime));
    }
}

bool ScriptingEngine::StopWorkerThread() {
    bool res = IsJoinable();
    if (res) {
        ScriptingEngine::s_Working = false;
        Join();
    }
    return res;
}

bool ScriptingEngine::IsJoinable() {
    return m_WorkerThread.joinable();
}

void ScriptingEngine::Join() {
    m_WorkerThread.join();
}

bool ScriptingEngine::FinishIfRequired() {
    if (IsJoinable()) {
        if (!ScriptingEngine::s_Working) {
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

ez::xml::Nodes ScriptingEngine::getXmlNodes(const std::string& /*vUserDatas*/) {
    ez::xml::Node node;
    return node.getChildren();
}

bool ScriptingEngine::setFromXmlNodes(const ez::xml::Node& vNode, const ez::xml::Node& vParent, const std::string& /*vUserDatas*/) {
    // The value of this child identifies the name of this element
    const auto& strName = vNode.getName();
    const auto& strValue = vNode.getContent();
    const auto& strParentName = vParent.getName();
    return true;
}

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
#include <project/ProjectFile.h>

#include <panes/ToolPane.h>
#include <panes/LogPane.h>

#include <ezlibs/ezFile.hpp>

#include <systems/PluginManager.h>

using namespace std::chrono;

///////////////////////////////////////////////////
/// STATIC ////////////////////////////////////////
///////////////////////////////////////////////////

static SourceFileID source_file_id = 0;
static SourceFileWeak source_file_parent;

///////////////////////////////////////////////////
/// STATIC'S //////////////////////////////////////
///////////////////////////////////////////////////

std::atomic<double> ScriptingEngine::s_progress(0.0);
std::atomic<bool> ScriptingEngine::s_working(false);
std::atomic<double> ScriptingEngine::s_generationTime(0.0);
std::mutex ScriptingEngine::s_workerThread_Mutex;

///////////////////////////////////////////////////
/// WORKER THREAD /////////////////////////////////
///////////////////////////////////////////////////

void ScriptingEngine::m_run(std::atomic<double>& vProgress, std::atomic<bool>& vWorking, std::atomic<double>& vGenerationTime) {
    vProgress = 0.0;

    vWorking = true;

    vGenerationTime = 0.0f;

    const int64_t firstTimeMark = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();

    Ltg::ScriptingModulePtr scriptingPtr = nullptr;
    Ltg::IDatasModelWeak datasModel;

    s_workerThread_Mutex.lock();

    const auto selectedScripting = m_scriptingModuleCombo.getText();
    if (m_scriptingModules.find(selectedScripting) != m_scriptingModules.end()) {
        scriptingPtr = m_scriptingModules.at(selectedScripting);
    }
    const auto scriptFilePathName = m_scriptFilePathName;
    const auto sourceFilePathNames = m_sourceFilePathNames;

    s_workerThread_Mutex.unlock();

    int32_t rowIndex = 0;  // the current line pos read from file
    int32_t rowCount = 0;  // the current line pos read from file

    if (!scriptFilePathName.empty()) {
        if (ez::file::isFileExist(scriptFilePathName)) {
            if (scriptingPtr->load(ScriptingEngine::Instance())) {
                Ltg::ErrorContainer errorContainer;
                if (!scriptingPtr->compileScript(scriptFilePathName, errorContainer)) {
                    LogVarLightError("Fail to compile script \"%s\"", scriptFilePathName.c_str());
                } else {
                    LogEngine::Instance()->Clear();
                    GraphView::Instance()->Clear();
                    DataBase::Instance()->OpenDBFile(ProjectFile::Instance()->m_ProjectFilePathName);
                    DataBase::Instance()->ClearDataTables();
                    for (const auto& sourceFilePathName : sourceFilePathNames) {
                        if (!sourceFilePathName.empty() && ez::file::isFileExist(sourceFilePathName)) {
                            const auto fileContent = ez::file::loadFileToString(sourceFilePathName);
                            if (!fileContent.empty()) {
                                try {
                                    source_file_id = DataBase::Instance()->AddSourceFile(sourceFilePathName);
                                    DataBase::Instance()->BeginTransaction();
                                    if (scriptingPtr->callScriptStart(errorContainer)) {
                                        const auto fileLines = ez::str::splitStringToVector(fileContent, '\n');
                                        rowCount = (int32_t)fileLines.size();
                                        scriptingPtr->setRowCount(rowCount);
                                        SetRowCount(rowCount);
                                        rowIndex = 0U;
                                        Ltg::ScriptingDatas datas;
                                        for (const auto& rowContent : fileLines) {
                                            if (!vWorking) {
                                                break;
                                            }
                                            const int64_t secondTimeMark = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
                                            vGenerationTime = (double)(secondTimeMark - firstTimeMark) / 1000.0;
                                            vProgress = (double)rowIndex / (double)rowCount;
                                            scriptingPtr->setRowIndex(rowIndex);
                                            SetRowIndex(rowIndex++);
                                            datas.buffer = rowContent;
                                            scriptingPtr->callScriptExec(datas, errorContainer);
                                        }
                                        scriptingPtr->callScriptEnd(errorContainer);
                                    }
                                    DataBase::Instance()->CommitTransaction();
                                } catch (std::exception& e) {
                                    LogVarLightError("%s", e.what());
                                    DataBase::Instance()->RollbackTransaction();
                                }
                            }
                        }
                    }
                    LogEngine::Instance()->Finalize();  // retrieve datas from database
                    DataBase::Instance()->CloseDBFile();
                }
                scriptingPtr->unload();
            }
        }
    }

    vWorking = false;
}

///////////////////////////////////////////////////
/// INIT/UNIT /////////////////////////////////////
///////////////////////////////////////////////////

void ScriptingEngine::Clear() {
    m_scriptFilePathName.clear();
    m_sourceFilePathNames.clear();
}

bool ScriptingEngine::Init() {
    m_fetchScriptingModules();
    return true;
}

void ScriptingEngine::Unit() {
    m_SelectedScriptingModule.reset();
    m_scriptingModules.clear();
    m_scriptingModuleCombo.clear();
}

void ScriptingEngine::SetInfos(const std::string& vInfos) {
    std::lock_guard<std::mutex> guard(s_workerThread_Mutex);
    m_scriptDescription = vInfos;
}

std::string ScriptingEngine::GetInfos() const {
    std::lock_guard<std::mutex> guard(s_workerThread_Mutex);
    return m_scriptDescription;
}

void ScriptingEngine::SetFunctionForEachRow(const std::string& vName) {
    std::lock_guard<std::mutex> guard(s_workerThread_Mutex);
    m_scriptFuncToCallForEachRow = vName;
}

std::string ScriptingEngine::GetFunctionForEachRow() const {
    std::lock_guard<std::mutex> guard(s_workerThread_Mutex);
    return m_scriptFuncToCallForEachRow;
}

void ScriptingEngine::SetFunctionForEndFile(const std::string& vName) {
    std::lock_guard<std::mutex> guard(s_workerThread_Mutex);
    m_scriptFuncToCallEndFile = vName;
}

std::string ScriptingEngine::GetFunctionForEndFile() const {
    std::lock_guard<std::mutex> guard(s_workerThread_Mutex);
    return m_scriptFuncToCallEndFile;
}

void ScriptingEngine::SetRowIndex(const int32_t& vRowID) {
    std::lock_guard<std::mutex> guard(s_workerThread_Mutex);
    m_scriptRowIndex = vRowID;
}

int32_t ScriptingEngine::GetRowIndex() const {
    std::lock_guard<std::mutex> guard(s_workerThread_Mutex);
    return m_scriptRowIndex;
}

void ScriptingEngine::SetRowCount(const int32_t& vRowCount) {
    std::lock_guard<std::mutex> guard(s_workerThread_Mutex);
    m_scriptRowCount = vRowCount;
}

int32_t ScriptingEngine::GetRowCount() const {
    std::lock_guard<std::mutex> guard(s_workerThread_Mutex);
    return m_scriptRowCount;
}

void ScriptingEngine::SetScriptFilePathName(const SourceFilePathName& vFilePathName) {
    m_scriptFilePathName = vFilePathName;
}

void ScriptingEngine::AddSourceFilePathName(const SourceFilePathName& vFilePathName) {
    m_sourceFilePathNames.push_back(vFilePathName);
}

void ScriptingEngine::AddSignalValue(const SignalCategory& vCategory,
                                     const SignalName& vName,
                                     const SignalEpochTime& vDate,
                                     const SignalValue& vValue,
                                     const SignalDesc& vDesc) {
    LogEngine::Instance()->AddSignalTick(source_file_parent, vCategory, vName, vDate, vValue, vDesc);
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

void ScriptingEngine::StartWorkerThread(const bool vFirstLoad) {
    if (!StopWorkerThread()) {
        if (!vFirstLoad) {
            LogEngine::Instance()->PrepareForSave();
        }
        LogEngine::Instance()->Clear();
        GraphView::Instance()->Clear();
        ToolPane::Instance()->Clear();
        LogPane::Instance()->Clear();
        ScriptingEngine::s_working = true;
        m_WorkerThread = std::thread(  //
            &ScriptingEngine::m_run,
            this,
            std::ref(ScriptingEngine::s_progress),
            std::ref(ScriptingEngine::s_working),
            std::ref(ScriptingEngine::s_generationTime));
    }
}

bool ScriptingEngine::StopWorkerThread() {
    bool res = IsJoinable();
    if (res) {
        ScriptingEngine::s_working = false;
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
        if (!ScriptingEngine::s_working) {
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

bool ScriptingEngine::drawMenu() {
    std::lock_guard<std::mutex> guard(s_workerThread_Mutex);
    if (m_scriptingModuleCombo.display(0.0f, "Scripting")) {
        m_selectScriptingModule(m_scriptingModuleCombo.getText());
    }
    return false;
}

bool ScriptingEngine::isValidScriptingSelected() const {
    std::lock_guard<std::mutex> guard(s_workerThread_Mutex);
    return (!m_SelectedScriptingModule.expired());
}

///////////////////////////////////////////////////////
//// SCRIPT LANG //////////////////////////////////////
///////////////////////////////////////////////////////

void ScriptingEngine::addSignalTag(double vEpoch, double r, double g, double b, double a, const std::string& vName, const std::string& vHelp) {
    if (vName.empty()) {
        LogVarLightError("%s", "Lua code error : the name is empty");
    } else {
        auto color = ImVec4(  //
            static_cast<float>(r),
            static_cast<float>(g),
            static_cast<float>(b),
            static_cast<float>(a));
        DataBase::Instance()->AddSignalTag(vEpoch, color, vName, vHelp);
    }
}

void ScriptingEngine::addSignalStatus(const std::string& vCategory, const std::string& vName, double vEpoch, const std::string& vStatus) {
    if (vCategory.empty() || vName.empty()) {
        if (vCategory.empty()) {
            LogVarLightError("%s", "Lua code error : the category passed to addSignalStatus is empty");
        }
        if (vName.empty()) {
            LogVarLightError("%s", "Lua code error : the name passed to addSignalStatus is empty");
        }
        return;
    }
    DataBase::Instance()->AddSignalStatus(source_file_id, vCategory, vName, vEpoch, vStatus, "");
}

void ScriptingEngine::addSignalValue(const std::string& vCategory, const std::string& vName, double vEpoch, double vValue, const std::string& vDesc) {
    if (vCategory.empty() || vName.empty()) {
        if (vCategory.empty()) {
            LogVarLightError("Lua code error : the category passed to addSignalValue(%s,%s,%f,%f,%s) is empty", //
                vCategory.c_str(), vName.c_str(), vEpoch, vValue, vDesc.c_str());
        }
        if (vName.empty()) {
            LogVarLightError("Lua code error : the name passed to addSignalValue(%s,%s,%f,%f,%s) is empty", //
                vCategory.c_str(), vName.c_str(), vEpoch, vValue, vDesc.c_str());
        }
        return;
    }
    DataBase::Instance()->AddSignalTick(source_file_id, vCategory, vName, vEpoch, vValue, vDesc);
}

void ScriptingEngine::addSignalStartZone(const std::string& vCategory, const std::string& vName, double vEpoch, const std::string& vStartMsg) {
    if (vCategory.empty() || vName.empty()) {
        if (vCategory.empty()) {
            LogVarLightError("%s", "Lua code error : the category passed to addSignalStartZone is empty");
        }
        if (vName.empty()) {
            LogVarLightError("%s", "Lua code error : the name passed to addSignalStartZone is empty");
        }
        return;
    }
    DataBase::Instance()->AddSignalStatus(source_file_id, vCategory, vName, vEpoch, vStartMsg, LogEngine::sc_START_ZONE);
}

void ScriptingEngine::addSignalEndZone(const std::string& vCategory, const std::string& vName, double vEpoch, const std::string& vEndMsg) {
    if (vCategory.empty() || vName.empty()) {
        if (vCategory.empty()) {
            LogVarLightError("%s", "Lua code error : the category passed to addSignalEndZone is empty");
        }
        if (vName.empty()) {
            LogVarLightError("%s", "Lua code error : the name passed to addSignalEndZone is empty");
        }
        return;
    }
    DataBase::Instance()->AddSignalStatus(source_file_id, vCategory, vName, vEpoch, vEndMsg, LogEngine::sc_END_ZONE);
}

///////////////////////////////////////////////////////
//// CONFIGURATION ////////////////////////////////////
///////////////////////////////////////////////////////

ez::xml::Nodes ScriptingEngine::getXmlNodes(const std::string& /*vUserDatas*/) {
    ez::xml::Node node;
    node.addChild("scripting_module").setContent(m_SelectedScriptingModuleName);
    return node.getChildren();
}

bool ScriptingEngine::setFromXmlNodes(const ez::xml::Node& vNode, const ez::xml::Node& vParent, const std::string& /*vUserDatas*/) {
    // The value of this child identifies the name of this element
    const auto& strName = vNode.getName();
    const auto& strValue = vNode.getContent();
    // const auto& strParentName = vParent.getName();
    if (strName == "scripting_module") {
        m_selectScriptingModule(strValue);
    }
    return true;
}

///////////////////////////////////////////////////////
//// PLUGINS //////////////////////////////////////////
///////////////////////////////////////////////////////

void ScriptingEngine::m_fetchScriptingModules() {
    m_scriptingModuleCombo.clear();
    m_scriptingModuleCombo.getArrayRef().push_back("None");
    auto modules = PluginManager::Instance()->getPluginModulesInfos();
    for (const auto& mod : modules) {
        if (mod.type == Ltg::PluginModuleType::SCRIPTING) {
            auto ptr = std::dynamic_pointer_cast<Ltg::ScriptingModule>(PluginManager::Instance()->createPluginModule(mod.label));
            if (ptr != nullptr) {
                m_scriptingModules[mod.label] = ptr;
                m_scriptingModuleCombo.getArrayRef().push_back(mod.label);
            }
        }
    }
}

void ScriptingEngine::m_selectScriptingModule(const Ltg::ScriptingModuleName& vName) {
    if (m_scriptingModules.find(vName) != m_scriptingModules.end()) {
        m_SelectedScriptingModule = m_scriptingModules.at(vName);
        ProjectFile::Instance()->SetProjectChange();
        m_scriptingModuleCombo.select(vName);
        m_SelectedScriptingModuleName = vName;
    }
}

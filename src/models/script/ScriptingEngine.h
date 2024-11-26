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

#pragma once

#include <map>
#include <mutex>
#include <thread>
#include <atomic>
#include <memory>
#include <vector>
#include <string>
#include <functional>
#include <Headers/DatasDef.h>
#include <apis/LtgPluginApi.h>

class ScriptingEngine : public Ltg::IDatasModel, public ez::xml::Config {
public:
    static std::mutex s_workerThread_Mutex;
    static std::atomic<bool> s_working;
    static std::atomic<double> s_progress;
    static std::atomic<double> s_generationTime;

private:                                       // script objects
    std::string m_scriptDescription;           // infos about script file
    std::string m_rowBufferContent;            // content of the buffer row
    std::string m_scriptFuncToCallForEachRow;  // the function to call for each lines
    std::string m_scriptFuncToCallEndFile;     // the fucntion to call for the end of the file
    int32_t m_scriptRowIndex = 0;              // the current row pos read from file
    int32_t m_scriptRowCount = 0;              // the row count read from file

private:  // Misc
    SourceFilePathName m_scriptFilePathName;
    std::vector<SourceFilePathName> m_sourceFilePathNames;
    std::map<Ltg::ScriptingModuleName, Ltg::ScriptingModulePtr> m_scriptingModules;
    ImWidgets::QuickStringCombo m_scriptingModuleCombo;
    Ltg::ScriptingModuleWeak m_SelectedScriptingModule;
    Ltg::ScriptingModuleName m_SelectedScriptingModuleName;

private:  // thread
    std::thread m_WorkerThread;

public:
    void Clear();

    bool Init();
    void Unit();

    void SetInfos(const std::string& vInfos);
    std::string GetInfos() const;

    void SetFunctionForEachRow(const std::string& vName);
    std::string GetFunctionForEachRow() const;

    void SetFunctionForEndFile(const std::string& vName);
    std::string GetFunctionForEndFile() const;

    void SetRowIndex(const int32_t& vRowID);
    int32_t GetRowIndex() const;

    void SetRowCount(const int32_t& vRowCount);
    int32_t GetRowCount() const;

    void SetScriptFilePathName(const SourceFilePathName& vFilePathName);

    void AddSourceFilePathName(const SourceFilePathName& vFilePathName);

    void AddSignalValue(  //
        const SignalCategory& vCategory,
        const SignalName& vName,
        const SignalEpochTime& vDate,
        const SignalValue& vValue);

    void AddSignalStatus(  //
        const SignalCategory& vCategory,
        const SignalName& vName,
        const SignalEpochTime& vDate,
        const SignalString& vString,
        const SignalStatus& vStatus);

    void StartWorkerThread(const bool& vFirstLoad);
    bool StopWorkerThread();
    bool IsJoinable();
    void Join();
    bool FinishIfRequired();

    bool drawMenu();
    bool isValidScriptingSelected() const;

    // interface with script languagesn so must be mutex protected
    void addSignalTag(double vEpoch, double r, double g, double b, double a, const std::string& vName, const std::string& vHelp) final;
    void addSignalStatus(const std::string& vCategory, const std::string& vName, double vEpoch, const std::string& vStatus) final;
    void addSignalValue(const std::string& vCategory, const std::string& vName, double vEpoch, double vValue) final;
    void addSignalStartZone(const std::string& vCategory, const std::string& vName, double vEpoch, const std::string& vStartMsg) final;
    void addSignalEndZone(const std::string& vCategory, const std::string& vName, double vEpoch, const std::string& vEndMsg) final;

private:
    void m_run(std::atomic<double>& vProgress, std::atomic<bool>& vWorking, std::atomic<double>& vGenerationTime);
    void m_fetchScriptingModules();
    void m_selectScriptingModule(const Ltg::ScriptingModuleName& vName);

public:  // configuration
    ez::xml::Nodes getXmlNodes(const std::string& vUserDatas = "") override;
    bool setFromXmlNodes(const ez::xml::Node& vNode, const ez::xml::Node& vParent, const std::string& vUserDatas) override;

public:  // singleton
    static std::shared_ptr<ScriptingEngine> Instance() {
        static std::shared_ptr<ScriptingEngine> _instance = std::make_shared<ScriptingEngine>();
        return _instance;
    }

public:
    ScriptingEngine() = default;                                           // Prevent construction
    ScriptingEngine(const ScriptingEngine&) = delete;                      // Prevent construction by copying
    ScriptingEngine& operator=(const ScriptingEngine&) { return *this; };  // Prevent assignment
    virtual ~ScriptingEngine() = default;                                  // Prevent unwanted destruction};
};

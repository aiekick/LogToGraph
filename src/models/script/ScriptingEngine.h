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
#include <Headers/Globals.h>

class ScriptingEngine : public ez::xml::Config {
public:
    static std::mutex s_WorkerThread_Mutex;
    static std::atomic<bool> s_Working;
    static std::atomic<double> s_Progress;
    static std::atomic<double> s_GenerationTime;
    static constexpr const char* sc_START_ZONE = "START_ZONE";
    static constexpr const char* sc_END_ZONE = "END_ZONE";

private:  // script objects
    std::string m_ScriptDescription;           // infos about script file
    std::string m_RowBufferContent;            // content of the buffer row
    std::string m_ScriptFuncToCallForEachRow;  // the function to call for each lines
    std::string m_ScriptFuncToCallEndFile;     // the fucntion to call for the end of the file
    int32_t m_ScriptRowIndex = 0;              // the current row pos read from file
    int32_t m_ScriptRowCount = 0;              // the row count read from file

private:// Misc
    SourceFilePathName m_ScriptFilePathName;
    std::vector<SourceFilePathName> m_SourceFilePathNames;

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

    void AddSignalValue(const SignalCategory& vCategory, const SignalName& vName, const SignalEpochTime& vDate, const SignalValue& vValue);
    void AddSignalStatus(const SignalCategory& vCategory,
                         const SignalName& vName,
                         const SignalEpochTime& vDate,
                         const SignalString& vString,
                         const SignalStatus& vStatus);

    void StartWorkerThread(const bool& vFirstLoad);
    bool StopWorkerThread();
    bool IsJoinable();
    void Join();
    bool FinishIfRequired();

private:
    void m_Run(std::atomic<double>& vProgress, std::atomic<bool>& vWorking, std::atomic<double>& vGenerationTime);
    bool m_compileScript(const std::string& vFilePathName);
    bool m_callScriptInit();
    bool m_callScriptExec(const std::string& vRow);
    bool m_callScriptEnd();

public:  // configuration
    ez::xml::Nodes getXmlNodes(const std::string& vUserDatas = "") override;
    bool setFromXmlNodes(const ez::xml::Node& vNode, const ez::xml::Node& vParent, const std::string& vUserDatas) override;

public:  // singleton
    static std::shared_ptr<ScriptingEngine> Instance() {
        static std::shared_ptr<ScriptingEngine> _instance = std::make_shared<ScriptingEngine>();
        return _instance;
    }

public:
    ScriptingEngine() = default;                                     // Prevent construction
    ScriptingEngine(const ScriptingEngine&) = delete;                      // Prevent construction by copying
    ScriptingEngine& operator=(const ScriptingEngine&) { return *this; };  // Prevent assignment
    virtual ~ScriptingEngine() = default;                            // Prevent unwanted destruction};
};

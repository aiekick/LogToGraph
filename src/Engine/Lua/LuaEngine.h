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

#include <mutex>
#include <thread>
#include <atomic>
#include <memory>
#include <string>
#include <functional>
#include <Headers/Globals.h>
#include <ctools/ConfigAbstract.h>

struct lua_State;
class LuaEngine : public conf::ConfigAbstract
{
public:
	static std::mutex s_WorkerThread_Mutex;
	static std::atomic<bool> s_Working;
	static std::atomic<double> s_Progress;
	static std::atomic<double> s_GenerationTime;
	static constexpr const char* sc_START_ZONE = "START_ZONE";
	static constexpr const char* sc_END_ZONE = "END_ZONE";

public:
	static void sSetLuaBufferVarContent(lua_State* vLuaState, const std::string& vVarName, const std::string& vContent);
	static void sLuAnalyse(std::atomic<double>& vProgress, std::atomic<bool>& vWorking, std::atomic<double>& vGenerationTime);

private: // lua objetcs
	lua_State* m_LuaStatePtr = nullptr;

	// lua vars, will be set by lua
	std::string m_Lua_Script_Description;					// infos about script file
	std::string m_Lua_Row_Buffer_Var_Name;					// row buffer var name
	std::string m_Lua_Row_Buffer_Content;					// content of the buffer row
	std::string m_Lua_Function_To_Call_For_Each_Line;		// the function to call for each lines
	std::string m_Lua_Function_To_Call_End_File;			// the fucntion to call for the end of the file
	int32_t m_Lua_Row_Index = 0;							// the current row pos read from file
	int32_t m_Lua_Row_Count = 0;							// the row count read from file

private: // thread
	std::thread m_WorkerThread;

private: // to save
	std::string m_LuaFilePathName;
	std::string m_LuaFileName;
	std::vector<std::pair<SourceFileName,SourceFilePathName>> m_SourceFilePathNames;

public:
	void Clear();

	bool Init();
	void Unit();

	bool ExecScriptCode(const std::string& vCode, std::string& vErrors);

	void SetInfos(const std::string& vInfos);
	std::string GetInfos();
	
	void SetRowBufferName(const std::string& vName);
	std::string GetRowBufferName();
	
	void SetFunctionForEachRow(const std::string& vName);
	std::string GetFunctionForEachRow();
	
	void SetFunctionForEndFile(const std::string& vName);
	std::string GetFunctionForEndFile();

	void SetRowIndex(const int32_t& vRowID);
	int32_t GetRowIndex() const;

	void SetRowCount(const int32_t& vRowCount);
	int32_t GetRowCount() const;

	void SetLuaFilePathName(const std::string& vFilePathName);
	std::string GetLuaFilePathName();
	std::string GetLuaFileName();

	void AddSourceFilePathName(const SourceFilePathName& vFilePathName);
	std::vector<std::pair<SourceFileName, SourceFilePathName>>& GetSourceFilePathNamesRef();

	void AddSignalValue(
		const SignalCategory& vCategory, 
		const SignalName& vName, 
		const SignalEpochTime& vDate, 
		const SignalValue& vValue);
	void AddSignalStatus(
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

public: // configuration
	std::string getXml(const std::string& vOffset, const std::string& vUserDatas) override;
	bool setFromXml(tinyxml2::XMLElement* vElem, tinyxml2::XMLElement* vParent, const std::string& vUserDatas) override;

public: // singleton
	static std::shared_ptr<LuaEngine> Instance()
	{
		static std::shared_ptr<LuaEngine> _instance = std::make_shared<LuaEngine>();
		return _instance;
	}

public:
	LuaEngine() = default; // Prevent construction
	LuaEngine(const LuaEngine&) = delete; // Prevent construction by copying
	LuaEngine& operator =(const LuaEngine&) { return *this; }; // Prevent assignment
	virtual ~LuaEngine() = default; // Prevent unwanted destruction};
};
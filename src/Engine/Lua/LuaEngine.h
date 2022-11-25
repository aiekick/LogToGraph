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

#pragma once

#include <mutex>
#include <thread>
#include <atomic>
#include <memory>
#include <string>
#include <functional>

#include <ctools/ConfigAbstract.h>

struct lua_State;
class LuaEngine : public conf::ConfigAbstract
{
public:
	static std::mutex s_WorkerThread_Mutex;
	static std::atomic<bool> s_Working;
	static std::atomic<double> s_Progress;
	static std::atomic<double> s_GenerationTime;

public:
	static void sSetLuaBufferVarContent(lua_State* vLuaState, const std::string& vVarName, const std::string& vContent);
	static void sLuAnalyse(std::atomic<double>& vProgress, std::atomic<bool>& vWorking, std::atomic<double>& vGenerationTime);

private:
	lua_State* m_LuaState = nullptr;

	// lua vars, will be set by lua
	std::string m_Lua_Infos;								// infos about script file
	std::string m_Lua_Current_Buffer_Line_Var_Name;			// current line of buffer
	std::string m_Lua_Last_Buffer_Line_Var_Name;			// last line of buffer
	std::string m_Lua_Current_Buffer_Line;					// current line of buffer
	std::string m_Lua_Last_Buffer_Line;						// last line of buffer
	std::string m_Lua_Function_To_Call_For_Each_Line;		// the function to call for each lines
	std::string m_Lua_Function_To_Call_End_File;			// the fucntion to call for the end of the file
	int32_t m_Lua_Current_Row_Line = 0U;					// the current line pos read from file

private: // thread
	std::thread m_WorkerThread;

private: // to save
	std::string m_LuaFilePathName;
	std::string m_LogFilePathName;

public:
	bool Init();
	void Unit();

	/*bool ExecScriptOnFile();*/
	bool ExecScriptCode(const std::string& vCode, std::string& vErrors);

	void SetInfos(const std::string& vInfos);
	std::string GetInfos();
	
	void SetBufferNameForCurrentLine(const std::string& vName);
	std::string GetBufferNameForCurrentLine();
	
	void SetBufferNameForLastLine(const std::string& vName);
	std::string GetBufferNameForLastLine();
	
	void SetFunctionForEachLine(const std::string& vName);
	std::string GetFunctionForEachLine();
	
	void SetFunctionForEndFile(const std::string& vName);
	std::string GetFunctionForEndFile();

	int32_t GetCurrentRowIndex();

	void SetLuaFilePathName(const std::string& vFilePathName);
	std::string GetLuaFilePathName();

	void SetLogFilePathName(const std::string& vFilePathName);
	std::string GetLogFilePathName();

	void StartWorkerThread(const bool& vFirstLoad);
	bool StopWorkerThread();
	bool IsJoinable();
	void Join();
	bool FinishIfRequired();

public: // configuration
	std::string getXml(const std::string& vOffset, const std::string& vUserDatas = "");
	bool setFromXml(tinyxml2::XMLElement* vElem, tinyxml2::XMLElement* vParent, const std::string& vUserDatas = "");

public: // singleton
	static std::shared_ptr<LuaEngine> Instance()
	{
		static std::shared_ptr<LuaEngine> _instance = std::make_shared<LuaEngine>();
		return _instance;
	}

public:
	LuaEngine() = default; // Prevent construction
	LuaEngine(const LuaEngine&) = default; // Prevent construction by copying
	LuaEngine& operator =(const LuaEngine&) { return *this; }; // Prevent assignment
	~LuaEngine() = default; // Prevent unwanted destruction};
};
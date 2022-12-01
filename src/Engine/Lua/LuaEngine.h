#pragma once

#include <memory>
#include <string>
#include <functional>

#include <ctools/ConfigAbstract.h>

struct lua_State;
class LuaEngine : public conf::ConfigAbstract
{
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

private: // to save
	std::string m_LuaFilePathName;
	std::string m_LogFilePathName;

public:
	bool Init();
	void Unit();

	bool ExecScriptOnFile();
	bool ExecScriptCode(const std::string& vCode, std::string& vErrors);

	void SetInfos(const std::string& vInfos);
	void SetBufferNameForCurrentLine(const std::string& vName);
	void SetBufferNameForLastLine(const std::string& vName);
	void SetFunctionForEachLine(const std::string& vName);
	void SetFunctionForEndFile(const std::string& vName);
	int32_t GetCurrentRowIndex();

	void SetLuaFilePathName(const std::string& vFilePathName);
	std::string GetLuaFilePathName();

	void SetLogFilePathName(const std::string& vFilePathName);
	std::string GetLogFilePathName();

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

private:
	void Set_Lua_BufferVar_Content(const std::string& vVarName, const std::string& vContent);
};
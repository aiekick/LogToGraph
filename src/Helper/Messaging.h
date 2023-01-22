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

#include <ctools/cTools.h>

#include <functional>
#include <cstdarg>
#include <string>
#include <utility> // std::pair
#include <vector>
#include <memory>

class MessageData
{
private:
	std::shared_ptr<void> puDatas;

public:
	MessageData() = default;
	MessageData(std::nullptr_t) {}
	template<typename T>
	MessageData(const std::shared_ptr<T>& vDatas)
	{
		SetUserDatas(vDatas);
	}
	template<typename T>
	void SetUserDatas(const std::shared_ptr<T>& vDatas)
	{
		puDatas = vDatas;
	}
	template<typename T>
	std::shared_ptr<T> GetUserDatas()
	{
		return std::static_pointer_cast<T>(puDatas);
	}
};

class ProjectManager;
class Messaging
{
public:
	static int sMessagePaneId;

private:
	const ImVec4 puErrorColor = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
	const ImVec4 puWarningColor = ImVec4(0.8f, 0.8f, 0.0f, 1.0f);

private:
	enum MessageTypeEnum
	{
		MESSAGE_TYPE_INFOS = 0,
		MESSAGE_TYPE_ERROR,
		MESSAGE_TYPE_WARNING
	};

	enum _MessageExistFlags
	{
		MESSAGE_EXIST_NONE = 0,
		MESSAGE_EXIST_INFOS = (1 << 0),
		MESSAGE_EXIST_ERROR = (1 << 1),
		MESSAGE_EXIST_WARNING = (1 << 2)
	};

	typedef int MessageExistFlags;
	MessageExistFlags puMessageExistFlags = MESSAGE_EXIST_INFOS | MESSAGE_EXIST_ERROR | MESSAGE_EXIST_WARNING;

	enum class SortingFieldEnum
	{
		FIELD_NONE = 0,
		FIELD_ID,
		FIELD_TYPE,
		FIELD_MSG
	} m_SortingField = SortingFieldEnum::FIELD_ID;

	std::string m_HeaderIdString;
	std::string m_HeaderTypeString;
	std::string m_HeaderMessageString;
	bool m_SortingDirection[3] = { true, true, true };	// true => Descending, false => Ascending

	int32_t currentMsgIdx = 0;
	typedef std::function<void(MessageData)> MessageFunc;
	typedef std::tuple<std::string, MessageTypeEnum, MessageData, MessageFunc> MessageBlock;
	std::vector<MessageBlock> puMessages;
	std::vector<MessageBlock> puFilteredMessages;

private:
	void AddMessage(const std::string& vMsg, MessageTypeEnum vType, bool vSelect, const MessageData& vDatas, const MessageFunc& vFunction);
	void AddMessage(MessageTypeEnum vType, bool vSelect, const MessageData& vDatas, const MessageFunc& vFunction, const char* fmt, va_list args);
	bool DrawMessage(const size_t& vMsgIdx);
	bool DrawMessage(const MessageBlock& vMsg);
	void SortFields(SortingFieldEnum vSortingField = SortingFieldEnum::FIELD_NONE, bool vCanChangeOrder = false);

	void UpdateFilteredMessages();
	void AddToFilteredMessages(const MessageBlock& vMessageBlock);
public:
	void DrawBar();
	void DrawConsole();
	void AddShaderMessage(const std::string& vErrorType, const std::string& vShaderTypeString, const std::string& vMessage);
	void AddInfos(const bool& vSelect, const MessageData& vDatas, const MessageFunc& vFunction, const char* fmt, ...); // select => set currentMsgIdx to this msg idx
	void AddWarning(const bool& vSelect, const MessageData& vDatas, const MessageFunc& vFunction, const char* fmt, ...); // select => set currentMsgIdx to this msg idx
	void AddError(const bool& vSelect, const MessageData& vDatas, const MessageFunc& vFunction, const char* fmt, ...); // select => set currentMsgIdx to this msg idx
	void ClearErrors();
	void ClearWarnings();
	void ClearInfos();
	void Clear();

public: // singleton
	static Messaging* Instance()
	{
		static Messaging _instance;
		return &_instance;
	}

protected:
	Messaging(); // Prevent construction
	Messaging(const Messaging&) {}; // Prevent construction by copying
	Messaging& operator =(const Messaging&) { return *this; }; // Prevent assignment
	~Messaging(); // Prevent unwanted destruction
};

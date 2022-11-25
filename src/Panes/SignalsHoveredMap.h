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

#include <Panes/Abstract/AbstractPane.h>
#include <Engine/Log/LogEngine.h>
#include <Headers/Globals.h>
#include <imgui/imgui.h>
#include <stdint.h>
#include <string>
#include <memory>
#include <map>

class ProjectFile;
class SignalsHoveredMap : public AbstractPane
{
private:
	ImGuiListClipper m_VirtualClipper;

public:
	bool Init() override;
	void Unit() override;
	int DrawPanes(const uint32_t& vCurrentFrame, const int& vWidgetId, const std::string& vUserDatas, PaneFlag& vInOutPaneShown) override;
	void DrawDialogsAndPopups(const uint32_t& vCurrentFrame, const std::string& vUserDatas) override;
	int DrawWidgets(const uint32_t& vCurrentFrame, const int& vWidgetId, const std::string& vUserDatas) override;

private:
	void DrawTable();
	int CalcSignalsButtonCountAndSize(ImVec2& vOutCellSize, ImVec2& vOutButtonSize);
	int DrawSignalButton(int& vWidgetPushId, SignalTickPtr vPtr, ImVec2 vGlyphSize);

public: // singleton
	static std::shared_ptr<SignalsHoveredMap> Instance()
	{
		static auto _instance = std::make_shared<SignalsHoveredMap>();
		return _instance;
	}

public:
	SignalsHoveredMap() = default; // Prevent construction
	SignalsHoveredMap(const SignalsHoveredMap&) = default; // Prevent construction by copying
	SignalsHoveredMap& operator =(const SignalsHoveredMap&) { return *this; }; // Prevent assignment
	~SignalsHoveredMap() = default; // Prevent unwanted destruction};
};

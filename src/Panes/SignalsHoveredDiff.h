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
class SignalsHoveredDiff : public AbstractPane
{
private:
	ImGuiListClipper m_VirtualClipper;
	std::vector<SignalTickWeak> m_PreviewTicks;

public:
	bool Init() override;
	void Unit() override;
	int DrawPanes(const uint32_t& vCurrentFrame, const int& vWidgetId, const std::string& vUserDatas, PaneFlag& vInOutPaneShown) override;
	void DrawDialogsAndPopups(const uint32_t& vCurrentFrame, const std::string& vUserDatas) override;
	int DrawWidgets(const uint32_t& vCurrentFrame, const int& vWidgetId, const std::string& vUserDatas) override;

private:
	void CheckItem(SignalTickPtr vSignalTick);
	void DrawTable();

public: // singleton
	static std::shared_ptr<SignalsHoveredDiff> Instance()
	{
		static auto _instance = std::make_shared<SignalsHoveredDiff>();
		return _instance;
	}

public:
	SignalsHoveredDiff() = default; // Prevent construction
	SignalsHoveredDiff(const SignalsHoveredDiff&) = default; // Prevent construction by copying
	SignalsHoveredDiff& operator =(const SignalsHoveredDiff&) { return *this; }; // Prevent assignment
	~SignalsHoveredDiff() = default; // Prevent unwanted destruction};
};

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
#include <imgui/imgui.h>
#include <stdint.h>
#include <string>
#include <memory>
#include <map>

class ProjectFile;
class GraphGroupPane : public AbstractPane
{
private:
	ImGuiListClipper m_FileListClipper;

public:
	bool Init() override;
	void Unit() override;
	int DrawPanes(const uint32_t& vCurrentFrame, const int& vWidgetId, const std::string& vUserDatas, PaneFlag& vInOutPaneShown) override;
	void DrawDialogsAndPopups(const uint32_t& vCurrentFrame, const std::string& vUserDatas) override;
	int DrawWidgets(const uint32_t& vCurrentFrame, const int& vWidgetId, const std::string& vUserDatas) override;

public: // singleton
	static std::shared_ptr<GraphGroupPane> Instance()
	{
		static auto _instance = std::make_shared<GraphGroupPane>();
		return _instance;
	}

public:
	GraphGroupPane() = default; // Prevent construction
	GraphGroupPane(const GraphGroupPane&) = default; // Prevent construction by copying
	GraphGroupPane& operator =(const GraphGroupPane&) { return *this; }; // Prevent assignment
	~GraphGroupPane() = default; // Prevent unwanted destruction};
};

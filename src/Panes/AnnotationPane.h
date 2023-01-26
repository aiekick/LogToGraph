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

#include <string>
#include <vector>
#include <map>
#include <imgui/imgui.h>
#include <Headers/Globals.h>
#include <ctools/ConfigAbstract.h>
#include <Panes/Abstract/AbstractPane.h>
#include <ImGuiFileDialog/ImGuiFileDialog.h>

class ProjectFile;
class AnnotationPane : public AbstractPane, public conf::ConfigAbstract
{
private:
	ImGuiListClipper m_AnnotationsListClipper;

public:
	bool Init() override;
	void Unit() override;
	int DrawPanes(const uint32_t& vCurrentFrame, const int& vWidgetId, const std::string& vUserDatas, PaneFlag& vInOutPaneShown) override;
	void DrawDialogsAndPopups(const uint32_t& vCurrentFrame, const std::string& vUserDatas) override;
	int DrawWidgets(const uint32_t& vCurrentFrame, const int& vWidgetId, const std::string& vUserDatas) override;

public:
	// configuration
	std::string getXml(const std::string& vOffset, const std::string& vUserDatas) override;
	bool setFromXml(tinyxml2::XMLElement* vElem, tinyxml2::XMLElement* vParent, const std::string& vUserDatas) override;
	
private:
	void DrawContent();
	void CheckItem(SignalSeriePtr vSignalSeriePtr);

public: // singleton
	static std::shared_ptr<AnnotationPane> Instance()
	{
		static auto _instance = std::make_shared<AnnotationPane>();
		return _instance;
	}

public:
	AnnotationPane() = default; // Prevent construction
	AnnotationPane(const AnnotationPane&) = delete; // Prevent construction by copying
	AnnotationPane& operator =(const AnnotationPane&) { return *this; }; // Prevent assignment
    virtual ~AnnotationPane() = default; // Prevent unwanted destruction};
};


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

#include <ImGuiPack.h>
#include <models/log/LogEngine.h>
#include <stdint.h>
#include <string>
#include <memory>
#include <map>

class ProjectFile;
class GraphListPane : public AbstractPane
{
private:
	ImGuiListClipper m_VirtualClipper;
	std::map<SignalCategory, std::vector<SignalSerieWeak>> m_CategorizedSignalSeries;
	std::vector<SignalSerieWeak> m_FilteredSignalSeries;
	char m_search_buffer[1024 + 1] = "";

public:
	void Clear();
	bool Init() override;
    void Unit() override;
    bool DrawPanes(const uint32_t& vCurrentFrame, bool* vOpened = nullptr, ImGuiContext* vContextPtr = nullptr, void* vUserDatas = nullptr) override;

	void UpdateDB();

public: // singleton
	static std::shared_ptr<GraphListPane> Instance()
	{
		static auto _instance = std::make_shared<GraphListPane>();
		return _instance;
	}

public:
	GraphListPane() = default; // Prevent construction
	GraphListPane(const GraphListPane&) = delete; // Prevent construction by copying
	GraphListPane& operator =(const GraphListPane&) { return *this; }; // Prevent assignment
    virtual ~GraphListPane() = default; // Prevent unwanted destruction};

private:
	void DisplayItem(const int& vIdx, const SignalSerieWeak& vDatasSerie);
	void DrawTree();
	void PrepareLog(const std::string& vSearchString);
	void HideAllGraphs();
};

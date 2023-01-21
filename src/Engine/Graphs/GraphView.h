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

#include <map>
#include <vector>
#include <memory>
#include <string>
#include <unordered_map>
#include <ctools/cTools.h>
#include <Headers/Globals.h>
#include <tinyxml2/tinyxml2.h>
#include <ctools/ConfigAbstract.h>
#include <implot/implot.h>

class GraphView final : public conf::ConfigAbstract
{
public:
	// https://www.shadertoy.com/view/ld3fzf
	static ct::fvec4 GetRainBow(const int32_t& vIdx, const int32_t& vCount);

private:
	GraphGroups m_GraphGroups;
	SignalValueRange m_Range_Value = SignalValueRange(0.5, -0.5) * DBL_MAX;
	bool m_show_hide_x_axis = true;
	bool m_show_hide_y_axis = false;
	bool m_need_show_hide_x_axis = false;
	bool m_need_show_hide_y_axis = false; 
	int32_t m_GraphsCount = 0;

public:
	void Clear();
	void AddSerieToGroup(const SignalSerieWeak& vSignalSerie, const size_t& vToGroupIdx);
	void RemoveSerieFromGroup(const SignalSerieWeak& vSignalSerie, const size_t& vFromGroupIdx);
	void MoveSerieFromGroupToGroup(const SignalSerieWeak& vSignalSerie, const size_t& vFromGroupIdx, const size_t& vToGroupIdx);
	GraphGroupsRef GetGraphGroups();

	void DrawGraphGroupTable();
	void DrawMenuBar();
	void DrawAloneGraphs(const GraphGroupPtr& vGraphGroupPtr, const ImVec2& vSize, bool& vFirstGraph);
	void DrawGroupedGraphs(const GraphGroupPtr& vGraphGroupPtr, const ImVec2& vSize, bool& vFirstGraph);

	void ComputeGraphsCount();
	int32_t GetGraphCount() const;

public:
	std::string getXml(const std::string& vOffset, const std::string& vUserDatas) override;
	bool setFromXml(tinyxml2::XMLElement* vElem, tinyxml2::XMLElement* vParent, const std::string& vUserDatas) override;

private:
	GraphGroupPtr prGetGroupAt(const size_t& vIdx);

	void prEraseGroupAt(const size_t& vIdx);
	void prDrawSignalGraph_ImPlot(const SignalSerieWeak& vSignalSerie, const ImVec2& vSize, const bool& vFirstGraph);

	bool prBeginPlot(const std::string& vLabel, ct::dvec2 vRangeValue, const ImVec2& vSize, const bool& vFirstGraph) const;
	static void prEndPlot();

public: // singleton
	static std::shared_ptr<GraphView> Instance()
	{
		static auto _instance = std::make_shared<GraphView>();
		return _instance;
	}

public:
	GraphView(); // Prevent construction
	GraphView(const GraphView&) = delete; // Prevent construction by copying
	GraphView& operator =(const GraphView&) { return *this; }; // Prevent assignment
    virtual ~GraphView() = default; // Prevent unwanted destruction};
};
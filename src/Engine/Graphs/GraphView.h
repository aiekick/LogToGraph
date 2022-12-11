#pragma once

#include <map>
#include <vector>
#include <memory>
#include <string>
#include <unordered_map>
#include <ctools/cTools.h>
#include <Headers/Globals.h>
#include <tinyxml2/tinyxml2.h>

class GraphView
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

public:
	void Clear();
	void AddSerieToGroup(SignalSerieWeak vSignalSerie, const size_t& vToGroupIdx);
	void RemoveSerieFromGroup(SignalSerieWeak vSignalSerie, const size_t& vFromGroupIdx);
	void MoveSerieFromGroupToGroup(SignalSerieWeak vSignalSerie, const size_t& vFromGroupIdx, const size_t& vToGroupIdx);
	GraphGroupsRef GetGraphGroups();

	void DrawGraphGroupTable();
	void DrawGraphs();

private:
	GraphGroupPtr prGetGroupAt(const size_t& vIdx);
	void prEraseGroupAt(const size_t& vIdx);
	void prDrawSignalGraph_ImPlot(SignalSerieWeak vSignalSerie);
	void prDrawGraph_ImPlot();
	void prDrawGraph_NewSystem();

public: // singleton
	static std::shared_ptr<GraphView> Instance()
	{
		static auto _instance = std::make_shared<GraphView>();
		return _instance;
	}

public:
	GraphView(); // Prevent construction
	GraphView(const GraphView&) = default; // Prevent construction by copying
	GraphView& operator =(const GraphView&) { return *this; }; // Prevent assignment
	~GraphView() = default; // Prevent unwanted destruction};
};
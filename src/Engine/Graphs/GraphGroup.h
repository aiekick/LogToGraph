#pragma once

#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <ctools/cTools.h>
#include <Headers/Globals.h>
#include <tinyxml2/tinyxml2.h>
#include <ctools/ConfigAbstract.h>
#include <Panes/Abstract/AbstractPane.h>

class GraphGroup : public conf::ConfigAbstract
{
public:
	static GraphGroupPtr Create(const uint32_t& vGroupIdx = 0U);

private:
	GraphGroupWeak m_This;
	SignalSeriesWeakContainer m_SignalSeries;
	SignalValueRange m_Range_Value = SignalValueRange(0.5, -0.5) * DBL_MAX;
	std::string m_Name;
	PaneFlag m_PaneFlag = 0U;

public:
	void Clear();
	void AddSignalSerie(SignalSerieWeak vSerie);
	void RemoveSignalSerie(SignalSerieWeak vSerie);
	SignalSeriesWeakContainerRef GetSignalSeries();
	SignalValueRangeConstRef GetSignalSeriesRange() const;
	void SetName(const std::string& vName);
	UInt8ConstPtr GetName();
	void SetPaneFlag(const PaneFlag& vPaneFlag);
	PaneFlag GetPaneFlag();

public:
	std::string getXml(const std::string& vOffset, const std::string& vUserDatas = "") override;
	bool setFromXml(tinyxml2::XMLElement* vElem, tinyxml2::XMLElement* vParent, const std::string& vUserDatas = "") override;

public: // singleton
	static std::shared_ptr<GraphGroup> Instance()
	{
		static auto _instance = std::make_shared<GraphGroup>();
		return _instance;
	}

public:
	GraphGroup() = default; // Prevent construction
	GraphGroup(const GraphGroup&) = default; // Prevent construction by copying
	GraphGroup& operator =(const GraphGroup&) { return *this; }; // Prevent assignment
	~GraphGroup() = default; // Prevent unwanted destruction};
};
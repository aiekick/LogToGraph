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
	static GraphGroupPtr Create();

private:
	GraphGroupWeak m_This;
	SignalSeriesWeakContainer m_SignalSeries;
	SignalValueRange m_Range_Value = SignalValueRange(0.5, -0.5) * DBL_MAX;
	std::string m_Name;

public:
	void Clear();
	void AddSignalSerie(const SignalSerieWeak& vSerie);
	void RemoveSignalSerie(const SignalSerieWeak& vSerie);
	SignalSeriesWeakContainerRef GetSignalSeries();
	SignalValueRangeConstRef GetSignalSeriesRange() const;
	void SetName(const std::string& vName);
	UInt8ConstPtr GetName();

public:
	std::string getXml(const std::string& vOffset, const std::string& vUserDatas = "") override;
	bool setFromXml(tinyxml2::XMLElement* vElem, tinyxml2::XMLElement* vParent, const std::string& vUserDatas = "") override;

private:
	void ComputeRange();

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
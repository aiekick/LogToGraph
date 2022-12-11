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

class SignalTick : public conf::ConfigAbstract
{
public:
	static SignalTickPtr Create();

private:
	SignalTickWeak m_This;

public:
	SignalEpochTime time_epoch;
	SignalDateTime time_date_time;
	SignalCategory category;
	SignalName name;
	SignalValue value;

public:
	std::string getXml(const std::string& vOffset, const std::string& vUserDatas = "") override;
	bool setFromXml(tinyxml2::XMLElement* vElem, tinyxml2::XMLElement* vParent, const std::string& vUserDatas = "") override;

public:
	SignalTick();
	~SignalTick();
};
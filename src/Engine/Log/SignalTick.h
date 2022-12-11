#pragma once

#include <map>
#include <vector>
#include <memory>
#include <string>
#include <unordered_map>
#include <ctools/cTools.h>
#include <Headers/Globals.h>
#include <tinyxml2/tinyxml2.h>


class SignalTick
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
	SignalTick();
	~SignalTick();
};
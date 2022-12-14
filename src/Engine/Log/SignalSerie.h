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

class SignalSerie : public conf::ConfigAbstract
{
public:
	static SignalSeriePtr Create();

public:
	SignalSerieWeak m_This;
	size_t count_base_records = 0U; // nombre d'enregistrements. on peut pas utiliser datas_values
	std::string low_case_name_for_search;
	SignalValueRange range_value = SignalValueRange(0.5, -0.5) * DBL_MAX;
	uint32_t graph_groupd_idx = 0U;
	std::vector<SignalTickWeak> datas_values;
	SignalCategory category;
	SignalName name;

	uint32_t color_u32 = ImGui::GetColorU32(ImVec4(0, 0, 0, 1));
	ImVec4 color_v4 = ImVec4(0,0,0,1);

public:
	std::string getXml(const std::string& vOffset, const std::string& vUserDatas = "") override;
	bool setFromXml(tinyxml2::XMLElement* vElem, tinyxml2::XMLElement* vParent, const std::string& vUserDatas = "") override;

public: // to save
	bool show = false;

public:
	SignalSerie();
	~SignalSerie();
	void InsertTick(const SignalTickWeak& vTick, const size_t& vIdx, const bool& vIncBaseRecordsCount = false);
	void AddTick(const SignalTickWeak& vTick, const bool& vIncBaseRecordsCount = false);
};
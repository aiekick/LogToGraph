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
	void AddTick(SignalTickWeak vTick, const bool& vIncBaseRecordsCount = false);
};
#include "LogEngine.h"
#include <ctools/cTools.h>
#include <ctools/Logger.h>
#include <Contrib/ImWidgets/ImWidgets.h>
#include <imgui/imgui_internal.h>


void SignalDatas::AddValue(const SignalTime& vTime, const SignalValue& vValue)
{
	min_date = ct::mini(min_date, vTime);
	max_date = ct::maxi(max_date, vTime);
	min_value = ct::mini(min_value, vValue);
	max_value = ct::maxi(max_value, vValue);
	datas_times.push_back(vTime);
	datas_values.push_back(vValue);
}

void LogEngine::Clear()
{
	m_GraphDatas.clear();
	m_LogDatas.clear();
}

void LogEngine::AddSignalValue(const std::string& vCategory, const std::string& vName, const double& vTime, const double& vValue)
{
	if (!vName.empty())
	{
		LogDatas logDatas;
		logDatas.category = vCategory;
		logDatas.name = vName;
		logDatas.time = vTime;
		logDatas.value = vValue;
		m_LogDatas.push_back(logDatas);

		if (m_DicoTimes.find(vTime) == m_DicoTimes.end())
		{
			m_DicoTimes[vTime] = m_ArrayTimes.size();
			m_ArrayTimes.push_back(vTime);
		}

		const auto current_count = m_DicoTimes.at(vTime);

		// ajout de la categorie
		auto& _datas_cat = m_GraphDatas[vCategory];

		if (_datas_cat.find(vName) == _datas_cat.end()) // first value of the signal
		{
			auto& _datas_name = _datas_cat[vName];
			_datas_name.category = vCategory;
			_datas_name.name = vName;

			for (size_t idx = 0U; idx < current_count; ++idx)
			{
				_datas_name.AddValue(m_ArrayTimes[idx], 0.0);
			}

			// puis on set la frame
			_datas_name.AddValue(vTime, vValue);
			_datas_name.low_case_name_for_search = ct::toLower(vName); // save low case signal name for search
			_datas_name.count_values++;
			_datas_name.show = false;
		}
		else // deja existant
		{
			auto& _datas_name = _datas_cat[vName];

			const auto last_local_pos = _datas_name.datas_times.size() - 1U;
			const auto global_pos = current_count - 1U;

			for (size_t idx = last_local_pos; idx < global_pos; ++idx)
			{
				_datas_name.AddValue(m_ArrayTimes[idx], _datas_name.datas_values[idx]);
			}

			// on set le time et la valeur de cette frame
			_datas_name.AddValue(vTime, vValue);
			_datas_name.count_values++;
			_datas_name.show = false;
		}
	}
}

void LogEngine::Finalize()
{
	// on va remplir la fin de cahque champs

	// last index
	const auto& last_index_global = m_DicoTimes.at(m_LogDatas.back().time);

	// on va ajouter les tick manquant a la fin
	for (auto& item_cat : *LogEngine::Instance())
	{
		for (auto& item_name : item_cat.second)
		{
			auto& datas = item_name.second;
			const auto& last_index_local = m_DicoTimes.at(datas.datas_times.back());
			if (last_index_global > last_index_local)
			{
				for (size_t idx = last_index_local; idx < last_index_global; ++idx)
				{
					datas.AddValue(m_ArrayTimes[idx], datas.datas_values[idx]);
				}
			}
		}
	}
}

SignalDatasContainer::iterator LogEngine::begin()
{
	return m_GraphDatas.begin();
}

SignalDatasContainer::iterator LogEngine::end()
{
	return m_GraphDatas.end();
}

void LogEngine::ShowHideSignal(const SignalCategory& vCategory, const SignalName& vName)
{
	if (m_GraphDatas.find(vCategory) != m_GraphDatas.end())
	{
		auto& cat = m_GraphDatas.at(vCategory);
		if (cat.find(vName) != cat.end())
		{
			cat.at(vName).show = !cat.at(vName).show;
		}
	}
}

void LogEngine::ShowHideSignal(const SignalCategory& vCategory, const SignalName& vName, const bool& vFlag)
{
	if (m_GraphDatas.find(vCategory) != m_GraphDatas.end())
	{
		auto& cat = m_GraphDatas.at(vCategory);
		if (cat.find(vName) != cat.end())
		{
			cat.at(vName).show = vFlag;
		}
	}
}

bool LogEngine::isSignalShown(const SignalCategory& vCategory, const SignalName& vName)
{
	if (m_GraphDatas.find(vCategory) != m_GraphDatas.end())
	{
		auto& cat = m_GraphDatas.at(vCategory);
		if (cat.find(vName) != cat.end())
		{
			return cat.at(vName).show;
		}
	}

	return false;
}

LogDatas& LogEngine::at(const size_t& vIdx)
{
	return m_LogDatas.at(vIdx);
}

size_t LogEngine::logDatasSize()
{
	return m_LogDatas.size();
}

void LogEngine::SetHoveredTime(const double& vValue)
{
	m_HoveredTime = vValue;
}

double LogEngine::GetHoveredTime()
{
	return m_HoveredTime;
}

void LogEngine::PrepareForSave()
{
	for (const auto& item_cat : m_GraphDatas)
	{
		for (const auto& item_name : item_cat.second)
		{
			m_SignalsShowingForSave[item_cat.first][item_name.first] = item_name.second.show;
		}
	}
}

void LogEngine::PrepareAfterLoad()
{
	for (const auto& item_cat : m_SignalsShowingForSave)
	{
		for (const auto& item_name : item_cat.second)
		{
			ShowHideSignal(item_cat.first, item_name.first, item_name.second);
		}
	}
}

bool LogEngine::setSignalVisibilty(tinyxml2::XMLElement* vElem, tinyxml2::XMLElement* vParent, const std::string& vUserDatas)
{
	// The value of this child identifies the name of this element
	std::string strName;
	std::string strValue;
	std::string strParentName;

	strName = vElem->Value();
	if (vElem->GetText())
		strValue = vElem->GetText();
	if (vParent != nullptr)
		strParentName = vParent->Value();

	if (vUserDatas == "project")
	{
		if (strParentName == "signals_visibility")
		{
			if (strName == "signal")
			{
				SignalCategory _signal_category;
				SignalName _signal_name;
				bool _signal_visibility = false;
				for (const tinyxml2::XMLAttribute* attr = vElem->FirstAttribute(); attr != nullptr; attr = attr->Next())
				{
					std::string attName = attr->Name();
					std::string attValue = attr->Value();

					if (attName == "category")
						_signal_category = attValue;
					else if (attName == "name")
						_signal_name = attValue;
					else if (attName == "visibility")
						_signal_visibility = ct::ivariant(attValue).GetB();
				}

				/// m_CurrentCategoryLoaded peut etre vide et c'est pas grave
				m_SignalsShowingForSave[_signal_category][_signal_name] = _signal_visibility;
			}
		}
	}

	return false;
}

std::string LogEngine::getSignalVisibilty(const std::string& vOffset, const std::string& /*vUserDatas*/)
{
	std::string str;

	str += vOffset + "<signals_visibility>\n";

	for (const auto& item_cat : m_SignalsShowingForSave)
	{
		for (const auto& item_name : item_cat.second)
		{
			if (item_name.second) // par defaut c'est false, alors on sauve que ceux qui sont visibles
			{
				str += vOffset + "\t<signal category = \"" + item_cat.first + 
					"\" name = \"" + item_name.first + 
					"\" visibility = \"" + (item_name.second ? "true" : "false") + "\" />\n";
			}
		}
	}

	str += vOffset + "</signals_visibility>\n";

	return str;
}
#include "LogEngine.h"
#include <ctime>
#include <ctools/cTools.h>
#include <ctools/Logger.h>
#include <Contrib/ImWidgets/ImWidgets.h>
#include <imgui/imgui_internal.h>

#include <Engine/Log/SignalSerie.h>
#include <Engine/Log/SignalTick.h>
#include <Engine/Graphs/GraphView.h>

void LogEngine::Clear()
{
	m_Range_ticks_time = SignalValueRange(0.5, -0.5) * DBL_MAX;
	m_SignalSeries.clear();
	m_SignalTicks.clear();
	m_VisibleCount = 0;
}

void LogEngine::AddSignalTick(const std::string& vCategory, const std::string& vName, const double& vTime, const double& vValue)
{
	if (!vName.empty())
	{
		auto tick_Ptr = SignalTick::Create();
		tick_Ptr->category = vCategory;
		tick_Ptr->name = vName;
		tick_Ptr->time_epoch = vTime;

		// 1668687822.067365000 => 17/11/2022 13:23:42.067365000
		double seconds = ct::fract(vTime); // 0.067365000
		std::time_t _epoch_time = (std::time_t)vTime;
		auto tm = std::localtime(&_epoch_time);
		double _sec = (double)tm->tm_sec + seconds;
		tick_Ptr->time_date_time = ct::toStr("%i/%i/%i %i:%i:%f",
			tm->tm_year + 1900, tm->tm_mon, tm->tm_mday, 
			tm->tm_hour, tm->tm_min, _sec);

		tick_Ptr->value = vValue;

		m_Range_ticks_time.x = ct::mini(m_Range_ticks_time.x, vTime);
		m_Range_ticks_time.y = ct::maxi(m_Range_ticks_time.y, vTime);

		m_SignalTicks.push_back(tick_Ptr);
		
		// ajout de la categorie
		auto& _datas_cat = m_SignalSeries[vCategory];

		if (_datas_cat.find(vName) == _datas_cat.end()) // first value of the signal
		{
			auto _datas_name_ptr = _datas_cat[vName] = SignalSerie::Create();
			if (_datas_name_ptr)
			{
				_datas_name_ptr->category = vCategory;
				_datas_name_ptr->name = vName;
				_datas_name_ptr->AddTick(tick_Ptr, true);
				_datas_name_ptr->low_case_name_for_search = ct::toLower(vName); // save low case signal name for search
				_datas_name_ptr->show = false;
			}
		}
		else // deja existant
		{
			auto _datas_name_ptr = _datas_cat.at(vName);
			if (_datas_name_ptr)
			{
				// on set le time et la valeur de cette frame
				_datas_name_ptr->AddTick(tick_Ptr, true);
				// by default not visible
				_datas_name_ptr->show = false;
			}
		}
	}
}

void LogEngine::Finalize()
{
	
}

void LogEngine::ShowHideSignal(const SignalCategory& vCategory, const SignalName& vName)
{
	if (m_SignalSeries.find(vCategory) != m_SignalSeries.end())
	{
		auto& cat = m_SignalSeries.at(vCategory);
		if (cat.find(vName) != cat.end())
		{
			auto ptr = cat.at(vName);
			if (ptr)
			{
				ptr->show = !ptr->show;
				m_VisibleCount += ptr->show ? 1 : -1;
				m_VisibleCount = ct::maxi(m_VisibleCount, 0);

				if (ptr->show)
				{
					GraphView::Instance()->AddSerieToGroup(ptr, 0U);
				}
				else
				{
					GraphView::Instance()->RemoveSerieFromGroup(ptr, ptr->graph_groupd_idx);
				}
			}
		}
	}
}

void LogEngine::ShowHideSignal(const SignalCategory& vCategory, const SignalName& vName, const bool& vFlag)
{
	if (m_SignalSeries.find(vCategory) != m_SignalSeries.end())
	{
		auto& cat = m_SignalSeries.at(vCategory);
		if (cat.find(vName) != cat.end())
		{
			auto ptr = cat.at(vName);
			if (ptr)
			{
				ptr->show = vFlag;
				m_VisibleCount += vFlag ? 1 : -1;
				m_VisibleCount = ct::maxi(m_VisibleCount, 0);

				if (ptr->show)
				{
					GraphView::Instance()->AddSerieToGroup(ptr, 0U);
				}
				else
				{
					GraphView::Instance()->RemoveSerieFromGroup(ptr, ptr->graph_groupd_idx);
				}
			}
		}
	}
}

bool LogEngine::isSignalShown(const SignalCategory& vCategory, const SignalName& vName, SignalColor* vOutColorPtr)
{
	if (m_SignalSeries.find(vCategory) != m_SignalSeries.end())
	{
		auto& cat = m_SignalSeries.at(vCategory);
		if (cat.find(vName) != cat.end())
		{
			auto ptr = cat.at(vName);
			if (ptr)
			{
				if (vOutColorPtr)
				{
					*vOutColorPtr = ptr->color_u32;
				}

				return ptr->show;
			}
		}
	}

	return false;
}

// get tick times
SignalValueRangeConstRef LogEngine::GetTicksTimeSerieRange() const
{ 
	return m_Range_ticks_time;
}

// get SignalTicksContainer
SignalTicksContainerRef LogEngine::GetSignalTicks()
{
	return m_SignalTicks; 
}
SignalSeriesContainerRef LogEngine::GetSignalSeries()
{ 
	return m_SignalSeries; 
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
	m_SignalsVisibility.clear();

	for (const auto& item_cat : m_SignalSeries)
	{
		for (const auto& item_name : item_cat.second)
		{
			if (item_name.second)
			{
				m_SignalsVisibility[item_cat.first][item_name.first] = item_name.second->show;
			}
		}
	}
}

void LogEngine::PrepareAfterLoad()
{
	m_VisibleCount = 0;

	for (const auto& item_cat : m_SignalsVisibility)
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
				m_SignalsVisibility[_signal_category][_signal_name] = _signal_visibility;
			}
		}
	}

	return false;
}

std::string LogEngine::getSignalVisibilty(const std::string& vOffset, const std::string& /*vUserDatas*/)
{
	std::string str;

	str += vOffset + "<signals_visibility>\n";

	for (const auto& item_cat : m_SignalsVisibility)
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

const int32_t& LogEngine::GetVisibleCount() const
{
	return m_VisibleCount; 
}

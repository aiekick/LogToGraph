#include "GraphGroup.h"

#include <Engine/Log/SignalSerie.h>
#include <Panes/Manager/LayoutManager.h>

GraphGroupPtr GraphGroup::Create(const uint32_t& vGroupIdx)
{
	auto res = std::make_shared<GraphGroup>();
	res->m_This = res;
	if (vGroupIdx)
	{
		uint32_t new_flag = LayoutManager::Instance()->GetRegisteredPaneCount() + vGroupIdx;
		if (new_flag < LayoutManager::Instance()->GetMaxPossiblePaneCount())
		{
			res->m_PaneFlag = (1 << new_flag);
			//res->m_Name = ct::toStr("Graphs Group %u##%u", (uint32_t)(vGroupIdx - 1U), (uintptr_t)res.get());
			res->m_Name = ct::toStr("Group %u", (uint32_t)(vGroupIdx - 1U));
			LayoutManager::Instance()->AddSpecificPaneToExisting(res->m_Name, GRAPH_PANE_NAME);
		}

		else
		{
			CTOOL_DEBUG_BREAK;
			res.reset();
		}
	}
	return res;
}

void GraphGroup::Clear()
{
	m_SignalSeries.clear();
	m_Range_Value = SignalValueRange(0.5, -0.5) * DBL_MAX;
}

void GraphGroup::AddSignalSerie(SignalSerieWeak vSerie)
{
	auto ptr = vSerie.lock();
	if (ptr)
	{
		m_Range_Value.x = ct::mini(m_Range_Value.x, ptr->range_value.x);
		m_Range_Value.y = ct::maxi(m_Range_Value.y, ptr->range_value.y);
		m_SignalSeries[ptr->category][ptr->name] = vSerie;
	}	
}

void GraphGroup::RemoveSignalSerie(SignalSerieWeak vSerie)
{
	auto ptr = vSerie.lock();
	if (ptr)
	{
		if (m_SignalSeries.find(ptr->category) != m_SignalSeries.end())
		{
			auto& ptr_cat = m_SignalSeries.at(ptr->category);

			if (ptr_cat.find(ptr->name) != ptr_cat.end())
			{
				ptr_cat.erase(ptr->name);

				// if the cat is empty we remove the cat
				if (ptr_cat.empty())
				{
					m_SignalSeries.erase(ptr->category);
				}
			}
		}
	}
}

SignalSeriesWeakContainerRef GraphGroup::GetSignalSeries()
{
	return m_SignalSeries;
}

SignalValueRangeConstRef GraphGroup::GetSignalSeriesRange() const
{
	return m_Range_Value;
}

void GraphGroup::SetName(const std::string& vName)
{
	m_Name = vName;
}

UInt8ConstPtr GraphGroup::GetName()
{
	return m_Name.c_str();
}

void GraphGroup::SetPaneFlag(const PaneFlag& vPaneFlag)
{
	m_PaneFlag = vPaneFlag;
}

PaneFlag GraphGroup::GetPaneFlag()
{
	return m_PaneFlag;
}

std::string GraphGroup::getXml(const std::string& vOffset, const std::string& /*vUserDatas*/)
{
	std::string str;

	return str;
}

bool GraphGroup::setFromXml(tinyxml2::XMLElement* vElem, tinyxml2::XMLElement* vParent, const std::string& /*vUserDatas*/)
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

	return true;
}
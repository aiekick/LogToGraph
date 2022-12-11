#include "GraphGroup.h"

#include <Engine/Log/SignalSerie.h>

GraphGroupPtr GraphGroup::Create()
{
	auto res = std::make_shared<GraphGroup>();
	res->m_This = res;
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

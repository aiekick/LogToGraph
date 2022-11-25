// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

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

#include "GraphGroup.h"

#include <Engine/Log/SignalSerie.h>
#include <Panes/Manager/LayoutManager.h>

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

void GraphGroup::AddSignalSerie(const SignalSerieWeak& vSerie)
{
	auto ptr = vSerie.lock();
	if (ptr)
	{
		m_SignalSeries[ptr->category][ptr->name] = vSerie;

		ComputeRange();
	}	
}

void GraphGroup::RemoveSignalSerie(const SignalSerieWeak& vSerie)
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

		ComputeRange();
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

std::string GraphGroup::getXml(const std::string& /*vOffset*/, const std::string& /*vUserDatas*/)
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

void GraphGroup::ComputeRange()
{
	m_Range_Value = SignalValueRange(0.5, -0.5) * DBL_MAX;

	for (auto& it_cat : m_SignalSeries)
	{
		for (auto& it_name : it_cat.second)
		{
			auto ptr = it_name.second.lock();
			if (ptr)
			{
				m_Range_Value.x = ct::mini(m_Range_Value.x, ptr->range_value.x);
				m_Range_Value.y = ct::maxi(m_Range_Value.y, ptr->range_value.y);
			}
		}
	}
}
// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/*
Copyright 2022-2023 Stephane Cuillerdier (aka aiekick)

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

#include "SignalSerie.h"
#include <models/log/SignalTick.h>
#include <models/graphs/GraphAnnotation.h>

SignalSeriePtr SignalSerie::Create()
{
	auto res = std::make_shared<SignalSerie>();
	res->m_This = res;
	return res;
}

void SignalSerie::InsertTick(const SignalTickWeak& vTick, const size_t& vIdx, const bool& vIncBaseRecordsCount)
{
	if (vIdx < datas_values.size())
	{
		auto ptr = vTick.lock();
		if (ptr)
		{
			ptr->parent = m_This;

			range_value.x = ez::mini(range_value.x, ptr->value);
			range_value.y = ez::maxi(range_value.y, ptr->value);
			datas_values.insert(datas_values.begin() + vIdx, vTick);

			if (vIncBaseRecordsCount)
			{
				++count_base_records;
			}
		}
	}
}

void SignalSerie::AddTick(const SignalTickWeak& vTick, const bool& vIncBaseRecordsCount)
{
	auto ptr = vTick.lock();
	if (ptr)
	{
		ptr->parent = m_This;

		range_value.x = ez::mini(range_value.x, ptr->value);
		range_value.y = ez::maxi(range_value.y, ptr->value);
		datas_values.push_back(vTick);

		if (vIncBaseRecordsCount)
		{
			++count_base_records;
		}
	}
}

void SignalSerie::AddGraphAnnotation(GraphAnnotationWeak vGraphAnnotation)
{
	m_GraphAnnotations.push_back(vGraphAnnotation);
}

void SignalSerie::DrawAnnotations()
{
	for (const auto& anno : m_GraphAnnotations)
	{
		auto ptr = anno.lock();
		if (ptr)
		{
			ptr->Draw();
		}
	}
}
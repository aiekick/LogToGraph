#include "SignalSerie.h"
#include <ctools/cTools.h>
#include <Engine/Log/SignalTick.h>

SignalSeriePtr SignalSerie::Create()
{
	auto res = std::make_shared<SignalSerie>();
	res->m_This = res;
	return res;
}

SignalSerie::SignalSerie()
{

}

SignalSerie::~SignalSerie()
{

}

void SignalSerie::AddTick(SignalTickWeak vTick, const bool& vIncBaseRecordsCount)
{
	auto ptr = vTick.lock();
	if (ptr)
	{
		range_value.x = ct::mini(range_value.x, ptr->value);
		range_value.y = ct::maxi(range_value.y, ptr->value);
		datas_values.push_back(vTick);

		if (vIncBaseRecordsCount)
		{
			++count_base_records;
		}
	}
}

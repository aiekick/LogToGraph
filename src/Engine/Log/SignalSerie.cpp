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

void SignalSerie::InsertTick(SignalTickWeak vTick, const size_t& vIdx, const bool& vIncBaseRecordsCount)
{
	if (vIdx < datas_values.size())
	{
		auto ptr = vTick.lock();
		if (ptr)
		{
			range_value.x = ct::mini(range_value.x, ptr->value);
			range_value.y = ct::maxi(range_value.y, ptr->value);
			datas_values.insert(datas_values.begin() + vIdx, vTick);

			if (vIncBaseRecordsCount)
			{
				++count_base_records;
			}
		}
	}
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

std::string SignalSerie::getXml(const std::string& vOffset, const std::string& /*vUserDatas*/)
{
	std::string str;

	return str;
}

bool SignalSerie::setFromXml(tinyxml2::XMLElement* vElem, tinyxml2::XMLElement* vParent, const std::string& /*vUserDatas*/)
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

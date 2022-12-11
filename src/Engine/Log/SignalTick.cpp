#include "SignalTick.h"

SignalTickPtr SignalTick::Create()
{
	auto res = std::make_shared<SignalTick>();
	res->m_This = res;
	return res;
}

SignalTick::SignalTick()
{

}

SignalTick::~SignalTick()
{

}

std::string SignalTick::getXml(const std::string& vOffset, const std::string& /*vUserDatas*/)
{
	std::string str;

	return str;
}

bool SignalTick::setFromXml(tinyxml2::XMLElement* vElem, tinyxml2::XMLElement* vParent, const std::string& /*vUserDatas*/)
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

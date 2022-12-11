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

#include <ctools/cTools.h>
#include <ctools/Logger.h>
#include <ctools/FileHelper.h>

#include <chrono>
#include <thread>

void AddSignalValue(const char* vLabel, const double& vValue)
{

}

class SignalGenerator
{
public:

};

void main()
{
	ct::ActionTime m_ActionTime_Power;
	ct::ActionTime m_ActionTime_Current;
	ct::ActionTime m_ActionTime_Transistor_1;
	ct::ActionTime m_ActionTime_User_Button;
	ct::ActionTime m_ActionTime_Led_1;
	ct::ActionTime m_ActionTime_Led_2;
	ct::ActionTime m_ActionTime_Led_3;

	size_t count_events = 1000U;
	//for (size_t idx = 0U; idx < count_events; ++idx)

	double time = 0.0; // time in sec
	while(true)
	{
		// timing of 1 ms
		std::this_thread::sleep_for(std::chrono::microseconds(1));

		// signal : power
		if (m_ActionTime_Power.IsTimeToAct(2000, true))
		{
			AddSignalValue("ALIM", sin(time)
		}

		// signal : current

		// signal : transistor 1

		// signal : user button

		// signal : led 1

		// signal : led 2

		// signal : led 3

		time += 1e-3f;
	}
}
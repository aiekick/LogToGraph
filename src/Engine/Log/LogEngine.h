#pragma once

#include <map>
#include <vector>
#include <memory>
#include <string>
#include <unordered_map>
#include <ctools/cTools.h>
#include <Headers/Globals.h>
#include <tinyxml2/tinyxml2.h>

class LogEngine
{
	// for searching, so no need the category
	typedef std::map<SignalName, SignalSerieWeak> OrderedCategoryLessSignalDatasContainer;

private:
	// containers of ptr's
	SignalSeriesContainer m_SignalSeries;
	SignalTicksContainer m_SignalTicks;

	// for display
	SignalValueRange m_Range_ticks_time = SignalValueRange(0.5, -0.5) * DBL_MAX;
	SignalEpochTime m_HoveredTime = 0.0;
	SignalCategory m_CurrentCategoryLoaded;

	int32_t m_VisibleCount = 0;

	// just for save signal visibility
	std::unordered_map<SignalName, std::unordered_map<SignalCategory, bool>> m_SignalsVisibility;

public:
	void Clear();
	void AddSignalTick(const SignalCategory& vCategory, const SignalName& vName, const SignalEpochTime& vDate, const SignalValue& vValue);
	void Finalize();

	// iter SignalDatasContainer
	void ShowHideSignal(const SignalCategory& vCategory, const SignalName& vName);
	void ShowHideSignal(const SignalCategory& vCategory, const SignalName& vName, const bool& vFlag);
	bool isSignalShown(const SignalCategory& vCategory, const SignalName& vName, SignalColor* vOutColorPtr = nullptr);

	// get tick times
	SignalValueRangeConstRef GetTicksTimeSerieRange() const;

	// get SignalTicksContainer
	SignalTicksContainerRef GetSignalTicks();
	SignalSeriesContainerRef GetSignalSeries();

	void SetHoveredTime(const double& vValue);
	double GetHoveredTime();

	void PrepareForSave();
	void PrepareAfterLoad();
	bool setSignalVisibilty(tinyxml2::XMLElement* vElem, tinyxml2::XMLElement* vParent, const std::string& /*vUserDatas*/);
	std::string getSignalVisibilty(const std::string& vOffset, const std::string& /*vUserDatas*/);
	const int32_t& GetVisibleCount() const;

public: // singleton
	static std::shared_ptr<LogEngine> Instance()
	{
		static auto _instance = std::make_shared<LogEngine>();
		return _instance;
	}

public:
	LogEngine() = default; // Prevent construction
	LogEngine(const LogEngine&) = default; // Prevent construction by copying
	LogEngine& operator =(const LogEngine&) { return *this; }; // Prevent assignment
	~LogEngine() = default; // Prevent unwanted destruction};
};
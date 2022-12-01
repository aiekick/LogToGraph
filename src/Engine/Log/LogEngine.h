#pragma once

#include <map>
#include <vector>
#include <memory>
#include <string>
#include <unordered_map>
#include <ctools/cTools.h>
#include <Headers/Globals.h>
#include <tinyxml2/tinyxml2.h>

struct SignalDatas
{
	size_t count_base_records = 0U; // nombre d'enregistrements. on peut pas utiliser datas_values
	std::string low_case_name_for_search;
	bool show = false;
	ct::dvec2 range_value = ct::dvec2(0.5, -0.5) * DBL_MAX;
	std::vector<SignalValue> datas_values;
	SignalCategory category;
	SignalName name;

	void AddValue(const SignalValue& vValue, const bool& vIncBaseRecordsCount = false);
};

typedef std::string SignalName;
typedef std::string SignalCategory;
typedef std::map<SignalCategory, std::map<SignalName, SignalDatas>> SignalDatasContainer;
typedef std::map<SignalName, SignalDatas> OrderedCategoryLessSignalDatasContainer; // pour l'affichage de la recherche, donc sans categorie

struct LogDatas
{
	SignalTime time;
	SignalCategory category;
	SignalName name;
	SignalValue value;
};

typedef std::vector<LogDatas> LogDatasContainer;

class LogEngine
{
private: // add vaalue in containers
	std::map<SignalTime, size_t> m_DicoTimes; // time to array index

private:
	SignalDatasContainer m_GraphValues;
	ct::dvec2 range_date = ct::dvec2(0.5, -0.5) * DBL_MAX;
	std::vector<SignalTime> m_GraphTimes; // array of time
	LogDatasContainer m_LogDatas;
	double m_HoveredTime = 0.0;
	std::unordered_map<SignalName, std::unordered_map<SignalCategory, bool>> m_SignalsShowingForSave;
	SignalCategory m_CurrentCategoryLoaded;

public:
	void Clear();
	void AddSignalValue(const SignalCategory& vCategory, const SignalName& vName, const SignalTime& vDate, const SignalValue& vValue);
	void Finalize();

	// iter SignalDatasContainer
	SignalDatasContainer::iterator begin();
	SignalDatasContainer::iterator end();
	void ShowHideSignal(const SignalCategory& vCategory, const SignalName& vName);
	void ShowHideSignal(const SignalCategory& vCategory, const SignalName& vName, const bool& vFlag);
	bool isSignalShown(const SignalCategory& vCategory, const SignalName& vName);

	// get times
	std::vector<SignalTime>& GetTimes() { return m_GraphTimes; }
	const ct::dvec2& GetTimeRange() const { return range_date; }

	// get LogDatasContainer
	LogDatas& at(const size_t& vIdx);
	size_t logDatasSize();

	void SetHoveredTime(const double& vValue);
	double GetHoveredTime();

	void PrepareForSave();
	void PrepareAfterLoad();
	bool setSignalVisibilty(tinyxml2::XMLElement* vElem, tinyxml2::XMLElement* vParent, const std::string& /*vUserDatas*/);
	std::string getSignalVisibilty(const std::string& vOffset, const std::string& /*vUserDatas*/);

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
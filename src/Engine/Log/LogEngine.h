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

#pragma once

#include <map>
#include <mutex>
#include <vector>
#include <memory>
#include <string>
#include <unordered_map>
#include <ctools/cTools.h>
#include <Headers/Globals.h>
#include <tinyxml2/tinyxml2.h>

struct SignalSetting
{
	bool visibility = false;
	uint32_t color = 0U;
	uint32_t group = 0U;
};

class LogEngine
{
private:
	// for searching, so no need the category
	typedef std::map<SignalName, SignalSerieWeak> OrderedCategoryLessSignalDatasContainer;

public:
    static std::string sConvertEpochToDateTimeString(const double& vTime);

private:
	// source file container
	SourceFilesContainer m_SourceFiles;

	// containers of ptr's
	SignalSeriesContainer m_SignalSeries;
	SignalTicksContainer m_SignalTicks;

	// ticks container who are not datas, because created virtually
	// like first and last ticks of some sinalg for being the same as global time
	SignalTicksContainer m_VirtualTicks;

	// for display
	SignalValueRange m_Range_ticks_time = SignalValueRange(0.5, -0.5) * DBL_MAX;
	SignalEpochTime m_HoveredTime = 0.0;
	SignalCategory m_CurrentCategoryLoaded;

	int32_t m_VisibleCount = 0;
	int32_t m_SignalsCount = 0;

	// just for save signal settings
	std::unordered_map<SignalName, std::unordered_map<SignalCategory, SignalSetting>> m_SignalSettings;

	// hovered preview ticks
	SignalTicksWeakContainer m_PreviewTicks;

	// diff check
	SignalTicksWeakContainer m_DiffFirstTicks; // first mark container
	SignalTicksWeakContainer m_DiffSecondTicks; // second mark container
	SignalDiffWeakContainer m_DiffResult; // diff result container

public:
	void Clear();
	SourceFileWeak SetSourceFile(const SourceFileName& vSourceFileName);
	void AddSignalTick(const SourceFileWeak& vSourceFile, const SignalCategory& vCategory, const SignalName& vName, const SignalEpochTime& vDate, const SignalValue& vValue);
	void AddSignalTick(const SourceFileWeak& vSourceFile, const SignalCategory& vCategory, const SignalName& vName, const SignalEpochTime& vDate, const SignalString& vString);
	void Finalize();

	// iter SignalDatasContainer
	void ShowHideSignal(const SignalCategory& vCategory, const SignalName& vName);
	void ShowHideSignal(const SignalCategory& vCategory, const SignalName& vName, const bool& vFlag);
	bool isSignalShown(const SignalCategory& vCategory, const SignalName& vName, SignalColor* vOutColorPtr = nullptr);

	SourceFilesContainerRef GetSourceFiles();
	SignalValueRangeConstRef GetTicksTimeSerieRange() const;
	SignalTicksContainerRef GetSignalTicks();
	SignalSeriesContainerRef GetSignalSeries();

	void SetHoveredTime(const SignalEpochTime& vSignalEpochTime);
	double GetHoveredTime() const;

	void UpdateVisibleSignalsColoring();

	SignalTicksWeakContainerRef GetPreviewTicks();

	void PrepareForSave();
	void PrepareAfterLoad();
	bool setSignalVisibilty(tinyxml2::XMLElement* vElem, tinyxml2::XMLElement* vParent, const std::string& /*vUserDatas*/);
	std::string getSignalVisibilty(const std::string& vOffset, const std::string& /*vUserDatas*/);
	const int32_t& GetVisibleCount() const;
	void SetSignalSetting(const SignalCategory& vCategory, const SignalName& vName, const SignalSetting& vSignalSetting);

	const int32_t& GetSignalsCount() const;

	void SetFirstDiffMark(const SignalEpochTime& vSignalEpochTime);
	void SetSecondDiffMark(const SignalEpochTime& vSignalEpochTime);
	void ComputeDiffResult();
	SignalDiffWeakContainerRef GetDiffResultTicks();

public: // singleton
	static std::shared_ptr<LogEngine> Instance()
	{
		static auto _instance = std::make_shared<LogEngine>();
		return _instance;
	}

public:
	LogEngine() = default; // Prevent construction
	LogEngine(const LogEngine&) = delete; // Prevent construction by copying
	LogEngine& operator =(const LogEngine&) { return *this; }; // Prevent assignment
    virtual ~LogEngine() = default; // Prevent unwanted destruction};
};
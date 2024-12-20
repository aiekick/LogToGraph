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
#include <stdint.h>
#include <unordered_map>
#include <headers/DatasDef.h>
#include <ezlibs/ezXmlConfig.hpp>

struct SignalSetting {
    bool visibility = false;
    uint32_t color = 0U;
    uint32_t group = 0U;
};

class LogEngine : public ez::xml::Config {
private:
    // for searching, so no need the category
    typedef std::map<SignalName, SignalSerieWeak> OrderedCategoryLessSignalDatasContainer;

public:
    static std::string sConvertEpochToDateTimeString(const double& vTime);
    static constexpr const char* sc_START_ZONE = "START_ZONE";
    static constexpr const char* sc_END_ZONE = "END_ZONE";

private:
    // source file container
    SourceFilesContainer m_SourceFiles;

    // containers of ptr's
    SignalSeriesContainer m_SignalSeries;
    SignalTicksContainer m_SignalTicks;
    SignalTagsContainer m_SignalTags;

    // ticks container who are not datas, because created virtually
    // like first and last ticks of some signals for being the same as global time
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
    SignalTicksWeakPreviewContainer m_PreviewTicks;

    // diff check
    SignalTicksWeakContainer m_DiffFirstTicks;   // first mark container
    SignalTicksWeakContainer m_DiffSecondTicks;  // second mark container
    SignalDiffWeakContainer m_DiffResult;        // diff result container

public:
    void Clear();
    SourceFileWeak SetSourceFile(const SourceFileName& vSourceFileName);
    void AddSignalTick(const SourceFileWeak& vSourceFile,
                       const SignalCategory& vCategory,
                       const SignalName& vName,
                       const SignalEpochTime& vDate,
                       const SignalValue& vValue,
                       const SignalDesc& vDesc);
    void AddSignalStatus(const SourceFileWeak& vSourceFile,
                         const SignalCategory& vCategory,
                         const SignalName& vName,
                         const SignalEpochTime& vDate,
                         const SignalString& vString,
                         const SignalStatus& vStatus);
    void AddSignalTag(const SignalEpochTime& vSignalEpochTime,
                      const SignalTagColor& vSignalTagColor,
                      const SignalTagName& vSignalTagName,
                      const SignalTagHelp& vSignalTagHelp);
    void Finalize();

    // iter SignalDatasContainer
    void ShowHideSignal(const SignalCategory& vCategory, const SignalName& vName);
    void ShowHideSignal(const SignalCategory& vCategory, const SignalName& vName, const bool vFlag);
    bool isSignalShown(const SignalCategory& vCategory, const SignalName& vName, SignalColor* vOutColorPtr = nullptr);
    bool isSomeSelection() const;

    SourceFilesContainerRef GetSourceFiles();
    SignalValueRangeConstRef GetTicksTimeSerieRange() const;
    SignalTicksContainerRef GetSignalTicks();
    SignalTagsContainerRef GetSignalTags();
    SignalSeriesContainerRef GetSignalSeries();

    void SetHoveredTime(const SignalEpochTime& vSignalEpochTime, const bool vForce = false);
    double GetHoveredTime() const;

    void UpdateVisibleSignalsColoring();

    SignalTicksWeakPreviewContainerRef GetPreviewTicks();

    void PrepareForSave();
    void PrepareAfterLoad();
    const int32_t& GetVisibleCount() const;
    void SetSignalSetting(const SignalCategory& vCategory, const SignalName& vName, const SignalSetting& vSignalSetting);

    ez::xml::Nodes getXmlNodes(const std::string& vUserDatas = "") override;
    bool setFromXmlNodes(const ez::xml::Node& vNode, const ez::xml::Node& vParent, const std::string& vUserDatas) override;

    const int32_t& GetSignalsCount() const;

    void SetFirstDiffMark(const SignalEpochTime& vSignalEpochTime);
    void SetSecondDiffMark(const SignalEpochTime& vSignalEpochTime);
    void ComputeDiffResult();
    SignalDiffWeakContainerRef GetDiffResultTicks();

public:  // singleton
    static std::shared_ptr<LogEngine> Instance() {
        static auto _instance = std::make_shared<LogEngine>();
        return _instance;
    }

public:
    LogEngine() = default;                                     // Prevent construction
    LogEngine(const LogEngine&) = delete;                      // Prevent construction by copying
    LogEngine& operator=(const LogEngine&) { return *this; };  // Prevent assignment
    virtual ~LogEngine() = default;                            // Prevent unwanted destruction};
};

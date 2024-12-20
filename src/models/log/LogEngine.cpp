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

#include "LogEngine.h"
#include <ctime>

#include <models/log/SignalSerie.h>
#include <models/log/SignalTick.h>
#include <models/log/SignalTag.h>
#include <models/log/SourceFile.h>
#include <models/graphs/GraphView.h>
#include <models/database/DataBase.h>

#include <panes/LogPane.h>
#include <panes/LogPaneSecondView.h>
#include <panes/GraphListPane.h>
#include <panes/ToolPane.h>

#include <project/ProjectFile.h>

#include <ezlibs/ezTools.hpp>

///////////////////////////////////////////////////
/// STATIC'S //////////////////////////////////////
///////////////////////////////////////////////////

std::string LogEngine::sConvertEpochToDateTimeString(const double& vTime) {
    // 1668687822.067365000 => 17/11/2022 13:23:42.067365000
    double seconds = ez::fract(vTime);  // 0.067365000
    auto _epoch_time = (std::time_t)vTime;
    auto tm = std::localtime(&_epoch_time);
    if (tm) {
        double _sec = (double)tm->tm_sec + seconds;
        return ez::str::toStr("%i/%i/%i %i:%i:%f", tm->tm_year + 1900, tm->tm_mon, tm->tm_mday, tm->tm_hour, tm->tm_min, _sec);
    }
    return "";
}

///////////////////////////////////////////////////
/// PUBLIC ////////////////////////////////////////
///////////////////////////////////////////////////

void LogEngine::Clear() {
    m_Range_ticks_time = SignalValueRange(0.5, -0.5) * DBL_MAX;
    m_SignalSeries.clear();
    m_SignalTicks.clear();
    m_SignalTags.clear();
    m_VirtualTicks.clear();
    m_PreviewTicks.clear();
    m_DiffFirstTicks.clear();
    m_DiffSecondTicks.clear();
    m_DiffResult.clear();
    m_SourceFiles.clear();
    m_VisibleCount = 0;
    m_SignalsCount = 0;
}

SourceFileWeak LogEngine::SetSourceFile(const SourceFileName& vSourceFileName) {
    SourceFileWeak res;

    if (!vSourceFileName.empty()) {
        if (m_SourceFiles.find(vSourceFileName) == m_SourceFiles.end())  // not found
        {
            res = m_SourceFiles[vSourceFileName] = SourceFile::Create(vSourceFileName);
        } else {
            res = m_SourceFiles.at(vSourceFileName);
        }
    }

    return res;
}

void LogEngine::AddSignalTick(const SourceFileWeak& vSourceFile,
                              const SignalCategory& vCategory,
                              const SignalName& vName,
                              const SignalEpochTime& vDate,
                              const SignalValue& vValue,
                              const SignalDesc& vDesc) {
    if (!vName.empty()) {
        auto tick_Ptr = SignalTick::Create();
        tick_Ptr->category = vCategory;
        tick_Ptr->name = vName;
        tick_Ptr->time_epoch = vDate;
        tick_Ptr->time_date_time = LogEngine::sConvertEpochToDateTimeString(vDate);
        tick_Ptr->value = vValue;
        tick_Ptr->desc = vDesc;

        m_Range_ticks_time.x = ez::mini(m_Range_ticks_time.x, vDate);
        m_Range_ticks_time.y = ez::maxi(m_Range_ticks_time.y, vDate);

        m_SignalTicks.push_back(tick_Ptr);

        // ajout de la categorie
        auto& _datas_cat = m_SignalSeries[vCategory];

        if (_datas_cat.find(vName) == _datas_cat.end())  // first value of the signal
        {
            ++m_SignalsCount;

            auto& _datas_name_ptr = _datas_cat[vName] = SignalSerie::Create();
            if (_datas_name_ptr) {
                _datas_name_ptr->category = vCategory;
                _datas_name_ptr->name = vName;
                _datas_name_ptr->addTick(tick_Ptr, true);
                _datas_name_ptr->low_case_name_for_search = ez::str::toLower(vName);  // save low case signal name for search
                _datas_name_ptr->show = false;
                _datas_name_ptr->m_SourceFileParent = vSourceFile;
            }
        } else  // deja existant
        {
            auto& _datas_name_ptr = _datas_cat.at(vName);
            if (_datas_name_ptr) {
                // on set le time et la valeur de cette frame
                _datas_name_ptr->addTick(tick_Ptr, true);
                // by default not visible
                _datas_name_ptr->show = false;
            }
        }
    }
}

void LogEngine::AddSignalStatus(const SourceFileWeak& vSourceFile,
                                const SignalCategory& vCategory,
                                const SignalName& vName,
                                const SignalEpochTime& vDate,
                                const SignalString& vString,
                                const SignalStatus& vStatus) {
    if (!vName.empty()) {
        auto tick_Ptr = SignalTick::Create();
        tick_Ptr->category = vCategory;
        tick_Ptr->name = vName;
        tick_Ptr->time_epoch = vDate;
        tick_Ptr->time_date_time = LogEngine::sConvertEpochToDateTimeString(vDate);
        tick_Ptr->string = vString;
        tick_Ptr->status = vStatus;

        m_Range_ticks_time.x = ez::mini(m_Range_ticks_time.x, vDate);
        m_Range_ticks_time.y = ez::maxi(m_Range_ticks_time.y, vDate);

        m_SignalTicks.push_back(tick_Ptr);

        // ajout de la categorie
        auto& _datas_cat = m_SignalSeries[vCategory];

        if (_datas_cat.find(vName) == _datas_cat.end())  // first value of the signal
        {
            ++m_SignalsCount;

            auto& _datas_name_ptr = _datas_cat[vName] = SignalSerie::Create();
            if (_datas_name_ptr) {
                _datas_name_ptr->category = vCategory;
                _datas_name_ptr->name = vName;
                _datas_name_ptr->addTick(tick_Ptr, true);
                _datas_name_ptr->low_case_name_for_search = ez::str::toLower(vName);  // save low case signal name for search
                _datas_name_ptr->show = false;
                _datas_name_ptr->m_SourceFileParent = vSourceFile;
                _datas_name_ptr->is_zone = true;
            }
        } else  // deja existant
        {
            auto& _datas_name_ptr = _datas_cat.at(vName);
            if (_datas_name_ptr) {
                // on set le time et la valeur de cette frame
                _datas_name_ptr->addTick(tick_Ptr, true);
                // by default not visible
                _datas_name_ptr->show = false;
            }
        }
    }
}

void LogEngine::AddSignalTag(const SignalEpochTime& vSignalEpochTime,
                             const SignalTagColor& vSignalTagColor,
                             const SignalTagName& vSignalTagName,
                             const SignalTagHelp& vSignalTagHelp) {
    if (!vSignalTagName.empty()) {
        auto tag_Ptr = SignalTag::Create();
        tag_Ptr->time_epoch = vSignalEpochTime;
        tag_Ptr->time_date_time = LogEngine::sConvertEpochToDateTimeString(vSignalEpochTime);
        tag_Ptr->color = vSignalTagColor;
        tag_Ptr->name = vSignalTagName;
        tag_Ptr->help = vSignalTagHelp;

        m_SignalTags.push_back(tag_Ptr);
    }
}

void LogEngine::Finalize() {
    // get sources
    std::map<SourceFileID, SourceFileWeak> _SourceFiles;
    DataBase::Instance()->GetSourceFiles([this, &_SourceFiles](const SourceFileID& vSourceFileID, const SourceFilePathName& vSourceFilePathName) {
        if (_SourceFiles.find(vSourceFileID) == _SourceFiles.end())  // not found
        {
            _SourceFiles[vSourceFileID] = SetSourceFile(vSourceFilePathName);
        }
    });

    // get datas
    DataBase::Instance()->GetDatas([this, &_SourceFiles](const SourceFileID& vSourceFileID,
                                                         const SignalEpochTime& vSignalEpochTime,
                                                         const SignalCategory& vSignalCategory,
                                                         const SignalName& vSignalName,
                                                         const SignalValue& vSignalValue,
                                                         const SignalString& vSignalString,
                                                         const SignalStatus& vSignalStatus,
                                                         const SignalDesc& vSignalDesc) {
        if (_SourceFiles.find(vSourceFileID) != _SourceFiles.end())  // found
        {
            auto source_file_parent_weak = _SourceFiles.at(vSourceFileID);

            if (vSignalString.empty()) {
                AddSignalTick(source_file_parent_weak, vSignalCategory, vSignalName, vSignalEpochTime, vSignalValue, vSignalDesc);
            } else {
                AddSignalStatus(source_file_parent_weak, vSignalCategory, vSignalName, vSignalEpochTime, vSignalString, vSignalStatus);
            }
        }
    });

    // get tags
    DataBase::Instance()->GetTags(
        [this](const SignalEpochTime& vSignalEpochTime, const SignalTagColor& vSignalTagColor, const SignalTagName& vSignalTagName, const SignalTagHelp& vSignalTagHelp) {
            AddSignalTag(vSignalEpochTime, vSignalTagColor, vSignalTagName, vSignalTagHelp);
        });

    // consolide
    if (!m_SignalTicks.empty() && m_SignalTicks.front() && m_SignalTicks.back()) {
        // first tick of all signals
        auto global_first_time_tick = m_SignalTicks.front()->time_epoch;

        // last tick of all signals
        auto global_last_time_tick = m_SignalTicks.back()->time_epoch;

        // we will add first and last tickes for all signals
        for (auto& item_cat : m_SignalSeries) {
            for (auto& item_name : item_cat.second) {
                if (item_name.second) {
                    auto local_first_tick_ptr = item_name.second->datas_values.front().lock();
                    auto local_last_tick_ptr = item_name.second->datas_values.back().lock();

                    if (local_first_tick_ptr && local_last_tick_ptr) {
                        // first tick
                        if (global_first_time_tick < local_first_tick_ptr->time_epoch) {
                            auto tick_Ptr = SignalTick::Create();
                            tick_Ptr->category = item_cat.first;
                            tick_Ptr->name = item_name.first;
                            tick_Ptr->time_epoch = global_first_time_tick;
                            tick_Ptr->time_date_time = LogEngine::sConvertEpochToDateTimeString(global_first_time_tick);
                            tick_Ptr->value =
                                (ProjectFile::Instance()->m_UsePredefinedZeroValue ? //
                                    ProjectFile::Instance()->m_PredefinedZeroValue : //
                                    local_first_tick_ptr->value);

                            m_VirtualTicks.push_back(tick_Ptr);  // for retain the shared_pointer
                            item_name.second->insertTick(tick_Ptr, 0U, false);
                        }

                        // last tick
                        if (global_last_time_tick > local_last_tick_ptr->time_epoch) {
                            auto tick_Ptr = SignalTick::Create();
                            tick_Ptr->category = item_cat.first;
                            tick_Ptr->name = item_name.first;
                            tick_Ptr->time_epoch = global_last_time_tick;
                            tick_Ptr->time_date_time = LogEngine::sConvertEpochToDateTimeString(global_first_time_tick);
                            tick_Ptr->value = local_last_tick_ptr->value;

                            m_VirtualTicks.push_back(tick_Ptr);  // for retain the shared_pointer
                            item_name.second->addTick(tick_Ptr, false);
                        }
                    }
                }
            }
        }
    }

    // finalize signals
    for (auto& signal_cnt : m_SignalSeries) {
        for (auto& signal : signal_cnt.second) {
            if (signal.second != nullptr) {
                signal.second->finalize();
            }
        }
    }

    LogPane::Instance()->Clear();
    LogPaneSecondView::Instance()->Clear();
    GraphListPane::Instance()->UpdateDB();
    ToolPane::Instance()->UpdateTree();
}

void LogEngine::ShowHideSignal(const SignalCategory& vCategory, const SignalName& vName) {
    if (m_SignalSeries.find(vCategory) != m_SignalSeries.end()) {
        auto& cat = m_SignalSeries.at(vCategory);
        if (cat.find(vName) != cat.end()) {
            auto& ptr = cat.at(vName);
            if (ptr) {
                ptr->show = !ptr->show;
                m_VisibleCount += ptr->show ? 1 : -1;
                m_VisibleCount = ez::maxi(m_VisibleCount, 0);

                if (ptr->show) {
                    GraphView::Instance()->AddSerieToDefaultGroup(ptr);
                } else {
                    GraphView::Instance()->RemoveSerieFromGroup(ptr, ptr->graph_groupd_ptr);
                }

                ProjectFile::Instance()->SetProjectChange();
                GraphView::Instance()->ComputeGraphsCount();
                UpdateVisibleSignalsColoring();
            }
        }
    }
}

void LogEngine::ShowHideSignal(const SignalCategory& vCategory, const SignalName& vName, const bool vFlag) {
    if (m_SignalSeries.find(vCategory) != m_SignalSeries.end()) {
        auto& cat = m_SignalSeries.at(vCategory);
        if (cat.find(vName) != cat.end()) {
            auto& ptr = cat.at(vName);
            if (ptr) {
                ptr->show = vFlag;
                m_VisibleCount += vFlag ? 1 : -1;
                m_VisibleCount = ez::maxi(m_VisibleCount, 0);

                if (ptr->show) {
                    GraphView::Instance()->AddSerieToDefaultGroup(ptr);
                } else {
                    GraphView::Instance()->RemoveSerieFromGroup(ptr, ptr->graph_groupd_ptr);
                }

                ProjectFile::Instance()->SetProjectChange();
                GraphView::Instance()->ComputeGraphsCount();
                UpdateVisibleSignalsColoring();
            }
        }
    }
}

bool LogEngine::isSignalShown(const SignalCategory& vCategory, const SignalName& vName, SignalColor* vOutColorPtr) {
    bool res = false;

    if (m_SignalSeries.find(vCategory) != m_SignalSeries.end()) {
        auto& cat = m_SignalSeries.at(vCategory);
        if (cat.find(vName) != cat.end()) {
            auto& ptr = cat.at(vName);
            if (ptr) {
                if (vOutColorPtr) {
                    *vOutColorPtr = ptr->color_u32;
                }

                res = ptr->show;
            }
        }
    }

    return res;
}

bool LogEngine::isSomeSelection() const {
    return (m_VisibleCount > 0);
}

SourceFilesContainerRef LogEngine::GetSourceFiles() {
    return m_SourceFiles;
}

SignalValueRangeConstRef LogEngine::GetTicksTimeSerieRange() const {
    return m_Range_ticks_time;
}

SignalTicksContainerRef LogEngine::GetSignalTicks() {
    return m_SignalTicks;
}

SignalTagsContainerRef LogEngine::GetSignalTags() {
    return m_SignalTags;
}

SignalSeriesContainerRef LogEngine::GetSignalSeries() {
    return m_SignalSeries;
}

void LogEngine::SetHoveredTime(const SignalEpochTime& vHoveredTime, const bool vForce) {
    if (vForce || m_HoveredTime != vHoveredTime) {
        m_HoveredTime = vHoveredTime;
        ProjectFile::Instance()->SetProjectChange();
        auto last_previewed_ticks = m_PreviewTicks;
        m_PreviewTicks.clear();
        m_PreviewTicks.reserve(m_SignalsCount);
        size_t visible_idx = 0U;
        for (auto& item_cat : m_SignalSeries) {
            for (auto& item_name : item_cat.second) {
                if (item_name.second) {
                    if (ProjectFile::Instance()->m_ShowVariableSignalsInHoveredListView && item_name.second->isConstant()) {
                        continue;
                    }
                    SignalTickPtr last_ptr = nullptr;
                    for (const auto& tick_weak : item_name.second->datas_values) {
                        auto ptr = tick_weak.lock();
                        if (last_ptr && vHoveredTime >= last_ptr->time_epoch && ptr != nullptr && vHoveredTime <= ptr->time_epoch) {
                            if (m_PreviewTicks.tryAdd(item_name.second->name, last_ptr)) {
                                if (ProjectFile::Instance()->m_AutoColorize) {
                                    auto parent_ptr = last_ptr->parent.lock();
                                    if (parent_ptr && parent_ptr->show) {
                                        parent_ptr->color_u32 = ImGui::GetColorU32(ez::getRainBowColor((int32_t)visible_idx, m_VisibleCount));
                                        parent_ptr->color_v4 = ImGui::ColorConvertU32ToFloat4(parent_ptr->color_u32);

                                        ++visible_idx;
                                    }
                                }
                                if (last_previewed_ticks.exist(item_name.second->name)) {
                                    auto last_previewed_tick_ptr = last_previewed_ticks.value(item_name.second->name).lock();
                                    if (last_previewed_tick_ptr != nullptr) {
                                        last_ptr->just_changed = ez::isDifferent(last_previewed_tick_ptr->value, last_ptr->value);
                                    }
                                }
                            }
                            break;
                        }
                        last_ptr = ptr;
                    }
                }
            }
        }
    }
}

double LogEngine::GetHoveredTime() const {
    return m_HoveredTime;
}

void LogEngine::UpdateVisibleSignalsColoring() {
    if (ProjectFile::Instance()->m_AutoColorize) {
        size_t visible_idx = 0U;
        for (auto& item_cat : m_SignalSeries) {
            for (auto& item_name : item_cat.second) {
                if (item_name.second) {
                    SignalTickPtr last_ptr = nullptr;
                    for (const auto& tick_weak : item_name.second->datas_values) {
                        if (last_ptr) {
                            auto parent_ptr = last_ptr->parent.lock();
                            if (parent_ptr && parent_ptr->show) {
                                parent_ptr->color_u32 = ImGui::GetColorU32(ez::getRainBowColor((int32_t)visible_idx, m_VisibleCount));
                                parent_ptr->color_v4 = ImGui::ColorConvertU32ToFloat4(parent_ptr->color_u32);
                                ++visible_idx;
                            }
                        }
                        last_ptr = tick_weak.lock();
                    }
                }
            }
        }
    }
}

SignalTicksWeakPreviewContainerRef LogEngine::GetPreviewTicks() {
    return m_PreviewTicks;
}

void LogEngine::PrepareForSave() {
    m_SignalSettings.clear();
    for (const auto& item_cat : m_SignalSeries) {
        for (const auto& item_name : item_cat.second) {
            if (item_name.second && item_name.second->show) {
                SignalSetting ss;
                ss.visibility = item_name.second->show;
                ss.color = item_name.second->color_u32;
                ss.group = (uint32_t)GraphView::Instance()->GetGroupID(item_name.second->graph_groupd_ptr);
                m_SignalSettings[item_cat.first][item_name.first] = ss;
            }
        }
    }
}

void LogEngine::PrepareAfterLoad() {
    m_VisibleCount = 0;

    for (const auto& item_cat : m_SignalSettings) {
        for (const auto& item_name : item_cat.second) {
            SetSignalSetting(item_cat.first, item_name.first, item_name.second);
        }
    }

    GraphView::Instance()->ComputeGraphsCount();
    SetFirstDiffMark(ProjectFile::Instance()->m_DiffFirstMark);
    SetSecondDiffMark(ProjectFile::Instance()->m_DiffSecondMark);
}

bool LogEngine::setFromXmlNodes(const ez::xml::Node& vNode, const ez::xml::Node& vParent, const std::string& vUserDatas) {
    // The value of this child identifies the name of this element
    const auto& strName = vNode.getName();
    const auto& strValue = vNode.getContent();
    const auto& strParentName = vParent.getName();

    if (vUserDatas == "project") {
        if (strParentName == "signals_settings") {
            if (strName == "signal") {
                SignalSetting ss;
                SignalName _signal_name;
                SignalCategory _signal_category;
                if (vNode.isAttributeExist("category"))
                    _signal_category = vNode.getAttribute("category");
                else if (vNode.isAttributeExist("name"))
                    _signal_name = vNode.getAttribute("name");
                else if (vNode.isAttributeExist("visibility"))
                    ss.visibility = ez::ivariant(vNode.getAttribute("visibility")).GetB();
                else if (vNode.isAttributeExist("color"))
                    ss.color = ez::ivariant(vNode.getAttribute("color")).GetU();
                else if (vNode.isAttributeExist("group"))
                    ss.group = ez::ivariant(vNode.getAttribute("group")).GetU();
                // m_CurrentCategoryLoaded peut etre vide et c'est pas grave
                m_SignalSettings[_signal_category][_signal_name] = ss;
            }
        }
    }

    return false;
}

ez::xml::Nodes LogEngine::getXmlNodes(const std::string& vUserDatas) {
    ez::xml::Node node("signals_settings");

    for (const auto& item_cat : m_SignalSettings) {
        for (const auto& item_name : item_cat.second) {
            if (item_name.second.visibility)  // par defaut c'est false, alors on sauve que ceux qui sont visibles
            {
                node.addChild("signal_category")
                    .addAttribute("name", item_name.first)
                    .addAttribute("visibility", item_name.second.visibility ? "true" : "false")
                    .addAttribute("color", ez::str::toStr(item_name.second.color))
                    .addAttribute("group", ez::str::toStr(item_name.second.group));
            }
        }
    }

    return {node};
}

const int32_t& LogEngine::GetVisibleCount() const {
    return m_VisibleCount;
}

void LogEngine::SetSignalSetting(const SignalCategory& vCategory, const SignalName& vName, const SignalSetting& vSignalSetting) {
    if (m_SignalSeries.find(vCategory) != m_SignalSeries.end()) {
        auto& cat = m_SignalSeries.at(vCategory);
        if (cat.find(vName) != cat.end()) {
            auto& ptr = cat.at(vName);
            if (ptr) {
                // show
                ptr->show = vSignalSetting.visibility;
                m_VisibleCount += vSignalSetting.visibility ? 1 : -1;
                m_VisibleCount = ez::maxi(m_VisibleCount, 0);

                // group
                if (ptr->show) {
                    GraphView::Instance()->AddSerieToGroupID(ptr, vSignalSetting.group);
                } else {
                    GraphView::Instance()->RemoveSerieFromGroup(ptr, ptr->graph_groupd_ptr);
                }

                // color
                ptr->color_u32 = vSignalSetting.color;
                ptr->color_v4 = ImGui::ColorConvertU32ToFloat4(ptr->color_u32);
            }
        }
    }
}

const int32_t& LogEngine::GetSignalsCount() const {
    return m_SignalsCount;
}

void LogEngine::SetFirstDiffMark(const SignalEpochTime& vSignalEpochTime) {
    ProjectFile::Instance()->m_DiffFirstMark = vSignalEpochTime;

    ProjectFile::Instance()->SetProjectChange();

    if (m_DiffFirstTicks.empty()) {
        m_DiffFirstTicks.resize(m_SignalsCount);
    }

    size_t idx = 0U;
    for (auto& item_cat : m_SignalSeries) {
        for (auto& item_name : item_cat.second) {
            if (item_name.second) {
                SignalTickPtr last_ptr = nullptr;
                for (const auto& tick_weak : item_name.second->datas_values) {
                    auto ptr = tick_weak.lock();
                    if (last_ptr && ProjectFile::Instance()->m_DiffFirstMark >= last_ptr->time_epoch && ptr &&
                        ProjectFile::Instance()->m_DiffFirstMark <= ptr->time_epoch) {
                        if (idx < (size_t)m_SignalsCount) {
                            m_DiffFirstTicks[idx] = last_ptr;
                        } else {
                            EZ_TOOLS_DEBUG_BREAK;
                        }

                        break;
                    }

                    last_ptr = ptr;
                }

                ++idx;
            }
        }
    }

    ComputeDiffResult();
}

void LogEngine::SetSecondDiffMark(const SignalEpochTime& vSignalEpochTime) {
    ProjectFile::Instance()->m_DiffSecondMark = vSignalEpochTime;

    ProjectFile::Instance()->SetProjectChange();

    if (m_DiffSecondTicks.empty()) {
        m_DiffSecondTicks.resize(m_SignalsCount);
    }

    size_t idx = 0U;
    for (auto& item_cat : m_SignalSeries) {
        for (auto& item_name : item_cat.second) {
            if (item_name.second) {
                SignalTickPtr last_ptr = nullptr;
                for (const auto& tick_weak : item_name.second->datas_values) {
                    auto ptr = tick_weak.lock();
                    if (last_ptr && ProjectFile::Instance()->m_DiffSecondMark >= last_ptr->time_epoch && ptr &&
                        ProjectFile::Instance()->m_DiffSecondMark <= ptr->time_epoch) {
                        if (idx < (size_t)m_SignalsCount) {
                            m_DiffSecondTicks[idx] = last_ptr;
                        } else {
                            EZ_TOOLS_DEBUG_BREAK;
                        }

                        break;
                    }

                    last_ptr = ptr;
                }

                ++idx;
            }
        }
    }

    ComputeDiffResult();
}

void LogEngine::ComputeDiffResult() {
    m_DiffResult.clear();

    if (ProjectFile::Instance()->m_DiffFirstMark > 0.0 && ProjectFile::Instance()->m_DiffSecondMark > 0.0) {
        if (!m_DiffFirstTicks.empty() && m_DiffFirstTicks.size() == m_DiffSecondTicks.size()) {
            m_DiffResult.reserve(m_SignalsCount);

            const auto& count_items = m_DiffFirstTicks.size();
            for (size_t idx = 0U; idx < count_items; ++idx) {
                const auto& first_ptr = m_DiffFirstTicks.at(idx).lock();
                const auto& second_ptr = m_DiffSecondTicks.at(idx).lock();
                if (first_ptr && second_ptr) {
                    if (first_ptr->name == second_ptr->name) {
                        if (ez::isDifferent(first_ptr->value, second_ptr->value)) {
                            m_DiffResult.emplace_back(first_ptr, second_ptr);
                        }
                    } else {
                        EZ_TOOLS_DEBUG_BREAK;
                    }
                }
            }
        }
    }
}

SignalDiffWeakContainerRef LogEngine::GetDiffResultTicks() {
    return m_DiffResult;
}

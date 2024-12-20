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

#include "GraphGroup.h"

#include <models/log/SignalSerie.h>
#include <headers/DatasDef.h>

GraphGroupPtr GraphGroup::Create() {
    auto res = std::make_shared<GraphGroup>();
    res->m_This = res;
    return res;
}

void GraphGroup::Clear() {
    m_SignalSeries.clear();
    m_Range_Value = SignalValueRange(0.5, -0.5) * DBL_MAX;
}

void GraphGroup::AddSignalSerie(const SignalSerieWeak& vSerie) {
    auto ptr = vSerie.lock();
    if (ptr) {
        m_SignalSeries[ptr->category][ptr->name] = vSerie;

        ComputeRange();
    }
}

void GraphGroup::RemoveSignalSerie(const SignalSerieWeak& vSerie) {
    auto ptr = vSerie.lock();
    if (ptr) {
        if (m_SignalSeries.find(ptr->category) != m_SignalSeries.end()) {
            auto& ptr_cat = m_SignalSeries.at(ptr->category);

            if (ptr_cat.find(ptr->name) != ptr_cat.end()) {
                ptr_cat.erase(ptr->name);

                // if the cat is empty we remove the cat
                if (ptr_cat.empty()) {
                    m_SignalSeries.erase(ptr->category);
                }
            }
        }

        ComputeRange();
    }
}

SignalSeriesWeakContainerRef GraphGroup::GetSignalSeries() {
    return m_SignalSeries;
}

SignalValueRangeConstRef GraphGroup::GetSignalSeriesRange() const {
    return m_Range_Value;
}

void GraphGroup::SetName(const std::string& vName) {
    m_Name = vName;
}

ImGuiLabel GraphGroup::GetImGuiLabel() {
    return m_Name.c_str();
}

void GraphGroup::ComputeRange() {
    m_Range_Value = SignalValueRange(0.5, -0.5) * DBL_MAX;

    double _offsetZoneY = 0.0;
    for (auto& it_cat : m_SignalSeries) {
        for (auto& it_name : it_cat.second) {
            auto ptr = it_name.second.lock();
            if (ptr) {
                if (ptr->is_zone) {
                    m_Range_Value.x = ez::mini(m_Range_Value.x, ptr->range_value.x + _offsetZoneY);
                    m_Range_Value.y = ez::maxi(m_Range_Value.y, ptr->range_value.y + _offsetZoneY + 1.0);
                    _offsetZoneY += 1.0;
                } else {
                    m_Range_Value.x = ez::mini(m_Range_Value.x, ptr->range_value.x);
                    m_Range_Value.y = ez::maxi(m_Range_Value.y, ptr->range_value.y);
                }
            }
        }
    }
}

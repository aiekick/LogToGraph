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
#include <memory>
#include <string>
#include <unordered_map>
#include <Headers/DatasDef.h>

class GraphGroup {
public:
    static GraphGroupPtr Create();

private:
    GraphGroupWeak m_This;
    SignalSeriesWeakContainer m_SignalSeries;
    SignalValueRange m_Range_Value = SignalValueRange(0.5, -0.5) * DBL_MAX;
    std::string m_Name;

public:
    void Clear();
    void AddSignalSerie(const SignalSerieWeak& vSerie);
    void RemoveSignalSerie(const SignalSerieWeak& vSerie);
    SignalSeriesWeakContainerRef GetSignalSeries();
    SignalValueRangeConstRef GetSignalSeriesRange() const;
    void SetName(const std::string& vName);
    ImGuiLabel GetImGuiLabel();

private:
    void ComputeRange();

public:  // singleton
    static std::shared_ptr<GraphGroup> Instance() {
        static auto _instance = std::make_shared<GraphGroup>();
        return _instance;
    }

public:
    GraphGroup() = default;                                      // Prevent construction
    GraphGroup(const GraphGroup&) = delete;                      // Prevent construction by copying
    GraphGroup& operator=(const GraphGroup&) { return *this; };  // Prevent assignment
    virtual ~GraphGroup() = default;                             // Prevent unwanted destruction};
};

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
#include <vector>
#include <memory>
#include <string>
#include <unordered_map>
#include <headers/DatasDef.h>
#include <ezlibs/ezVec2.hpp>

class SignalSerie {
public:
    static SignalSeriePtr Create();

public:
    SignalSerieWeak m_This;
    SourceFileWeak m_SourceFileParent;
    size_t count_base_records = 0U;  // nombre d'enregistrements. on peut pas utiliser datas_values
    std::string low_case_name_for_search;
    SignalValueRange range_value = SignalValueRange(0.5, -0.5) * DBL_MAX;
    GraphGroupPtr graph_groupd_ptr = nullptr;
    std::vector<SignalTickWeak> datas_values;
    SignalCategory category;
    SignalName name;
    bool is_zone = false;
    std::string label; // label displayed by imgui
    uint32_t color_u32 = ImGui::GetColorU32(ImVec4(0, 0, 0, 1));
    ImVec4 color_v4 = ImVec4(0, 0, 0, 1);

    // for measuring / annotation
    bool hovered_by_mouse = false;
    std::vector<GraphAnnotationWeak> m_GraphAnnotations;
    bool show_hide_temporary = true;  // jsut hidden temporarily but always shown

public:                 // to save
    bool show = false;  // signal must be shown on graph screen

public:
    void insertTick(const SignalTickWeak& vTick, const size_t& vIdx, const bool vIncBaseRecordsCount = false);
    void addTick(const SignalTickWeak& vTick, const bool vIncBaseRecordsCount = false);

    void addGraphAnnotation(GraphAnnotationWeak vGraphAnnotation);

    void drawAnnotations();

    bool isConstant();
    void finalize();
};

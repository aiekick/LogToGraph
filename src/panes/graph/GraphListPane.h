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

#include <ezlibs/ezClass.hpp>
#include <ezlibs/ezSingleton.hpp>
#include <imguipack.h>
#include <models/log/LogEngine.h>
#include <stdint.h>
#include <string>
#include <memory>
#include <map>

class ProjectFile;
class GraphListPane : public AbstractPane {
    DISABLE_CONSTRUCTORS(GraphListPane)
    DISABLE_DESTRUCTORS(GraphListPane)
    IMPLEMENT_SHARED_SINGLETON(GraphListPane)

private:
    ImGuiListClipper m_VirtualClipper;
    std::map<SignalCategory, std::vector<SignalSerieWeak>> m_CategorizedSignalSeries;
    std::vector<SignalSerieWeak> m_FilteredSignalSeries;
    char m_search_buffer[1024 + 1] = "";

public:
    void Clear();
    bool init() override;
    void unit() override;
    bool drawPanes(bool* apOpened, LayoutPaneUserDatas apUserDatas) override;

    void UpdateDB();

private:
    void DisplayItem(const int& vIdx, const SignalSerieWeak& vDatasSerie);
    void DrawMenuBar();
    void DrawTree();
    void PrepareLog(const std::string& vSearchString);
    void HideAllGraphs();
};

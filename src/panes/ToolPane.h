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
#include <cstdint>
#include <headers/DatasDef.h>
#include <ezlibs/ezClass.hpp>
#include <ezlibs/ezSingleton.hpp>
#include <imguipack.h>
#include <models/log/LogEngine.h>
#include <models/log/SignalTree.h>

class ProjectFile;
class ToolPane : public AbstractPane {
    DISABLE_CONSTRUCTORS(ToolPane)
    DISABLE_DESTRUCTORS(ToolPane)
    IMPLEMENT_SHARED_SINGLETON(ToolPane)

private:
    ImGuiListClipper m_FileListClipper;
    char m_search_buffer[1024 + 1] = "";
    int32_t m_CurrentSourceEdited = -1;
    SignalTree m_SignalTree;

public:
    void Clear();
    bool init() override;
    void unit() override;
    bool drawPanes(bool* apOpened, LayoutPaneUserDatas apUserDatas) override;
    bool drawDialogsAndPopups(const ImRect& aRect, LayoutPaneUserDatas apUserDatas) override;

    void UpdateTree();

private:
    void DrawTable();
    void DrawTree();
    static void HideAllGraphs();
};

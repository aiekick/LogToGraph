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
#include <Headers/Globals.h>
#include <models/log/LogEngine.h>

class ProjectFile;
class ToolPane : public AbstractPane {
private:
    ImGuiListClipper m_FileListClipper;
    std::map<SignalName, SignalSerieWeak> m_SignalSeries;
    char m_search_buffer[1024 + 1] = "";
    int32_t m_CurrentSourceEdited = -1;

public:
    void Clear();
    bool Init() override;
    void Unit() override;
    bool DrawPanes(const uint32_t& vCurrentFrame, bool* vOpened = nullptr, ImGuiContext* vContextPtr = nullptr, void* vUserDatas = nullptr) override;
    bool DrawDialogsAndPopups(const uint32_t& /*vCurrentFrame*/, const ImRect& /*vRect*/, ImGuiContext* /*vContextPtr*/, void* /*vUserDatas*/) override;

    void UpdateTree();

public:  // singleton
    static std::shared_ptr<ToolPane> Instance() {
        static auto _instance = std::make_shared<ToolPane>();
        return _instance;
    }

public:
    ToolPane() = default;                                    // Prevent construction
    ToolPane(const ToolPane&) = delete;                      // Prevent construction by copying
    ToolPane& operator=(const ToolPane&) { return *this; };  // Prevent assignment
    virtual ~ToolPane() = default;                           // Prevent unwanted destruction};

private:
    void DrawTable();
    static void DisplayItem(const SignalSerieWeak& vDatasSerie);
    void DrawTree();
    void PrepareLogAfterSearch(const std::string& vSearchString);
    static void HideAllGraphs();
};

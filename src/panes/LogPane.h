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

#include <ImGuiPack.h>
#include <models/log/LogEngine.h>
#include <Headers/DatasDef.h>
#include <stdint.h>
#include <string>
#include <memory>
#include <map>

class ProjectFile;
class LogPane : public AbstractPane {
private:
    ImGuiListClipper m_LogListClipper;
    SignalTicksWeakContainer m_LogDatas;
    std::vector<double> m_ValuesToHide;
    bool m_need_re_preparation = false;

public:
    bool Init() override;
    void Unit() override;
    bool DrawPanes(const uint32_t& vCurrentFrame, bool* vOpened = nullptr, ImGuiContext* vContextPtr = nullptr, void* vUserDatas = nullptr) override;

    void Clear();
    void CheckItem(SignalTickPtr vSignalTick);
    void PrepareLog();

public:  // singleton
    static std::shared_ptr<LogPane> Instance() {
        static auto _instance = std::make_shared<LogPane>();
        return _instance;
    }

public:
    LogPane() = default;                                   // Prevent construction
    LogPane(const LogPane&) = delete;                      // Prevent construction by copying
    LogPane& operator=(const LogPane&) { return *this; };  // Prevent assignment
    virtual ~LogPane() = default;                          // Prevent unwanted destruction};

private:
    void DrawTable();
};

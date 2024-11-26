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
class SignalsHoveredDiff : public AbstractPane {
private:
    ImGuiListClipper m_VirtualClipper;
    std::vector<SignalTickWeak> m_PreviewTicks;

public:
    void Clear();
    bool Init() override;
    void Unit() override;
    bool DrawPanes(const uint32_t& vCurrentFrame, bool* vOpened = nullptr, ImGuiContext* vContextPtr = nullptr, void* vUserDatas = nullptr) override;

private:
    static void CheckItem(const SignalTickPtr& vSignalTick);
    void DrawTable();

public:  // singleton
    static std::shared_ptr<SignalsHoveredDiff> Instance() {
        static auto _instance = std::make_shared<SignalsHoveredDiff>();
        return _instance;
    }

public:
    SignalsHoveredDiff() = default;                                              // Prevent construction
    SignalsHoveredDiff(const SignalsHoveredDiff&) = delete;                      // Prevent construction by copying
    SignalsHoveredDiff& operator=(const SignalsHoveredDiff&) { return *this; };  // Prevent assignment
    virtual ~SignalsHoveredDiff() = default;                                     // Prevent unwanted destruction};
};

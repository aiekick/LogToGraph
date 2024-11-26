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
class SignalsHoveredList : public AbstractPane {
private:
    ImGuiListClipper m_VirtualClipper;

public:
    void Clear();
    bool Init() override;
    void Unit() override;
    bool DrawPanes(const uint32_t& vCurrentFrame, bool* vOpened = nullptr, ImGuiContext* vContextPtr = nullptr, void* vUserDatas = nullptr) override;

    void SetHoveredTime(const SignalEpochTime& vHoveredTime);

private:
    void DrawTable();
    static int CalcSignalsButtonCountAndSize(ImVec2& vOutCellSize, ImVec2& vOutButtonSize);
    int DrawSignalButton(int& vWidgetPushId, SignalTickPtr vPtr, ImVec2 vGlyphSize);

public:  // singleton
    static std::shared_ptr<SignalsHoveredList> Instance() {
        static auto _instance = std::make_shared<SignalsHoveredList>();
        return _instance;
    }

public:
    SignalsHoveredList() = default;                                              // Prevent construction
    SignalsHoveredList(const SignalsHoveredList&) = delete;                      // Prevent construction by copying
    SignalsHoveredList& operator=(const SignalsHoveredList&) { return *this; };  // Prevent assignment
    virtual ~SignalsHoveredList() = default;                                     // Prevent unwanted destruction};
};

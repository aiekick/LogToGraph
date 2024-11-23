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

#include <cstdint>
#include <memory>

typedef int64_t LayoutPaneFlag;  // int64_t give 64 Panes max.
typedef std::string LayoutPaneName;

struct ImGuiContext;
struct ImVec2;
struct ImRect;

class ILayoutPane {
private:
    LayoutPaneName paneName;
    LayoutPaneFlag paneFlag = -1;

public:
    virtual bool Init() = 0;  // return false if the init was failed
    virtual void Unit() = 0;

    // the return, is a user side use case here
    virtual bool DrawPanes(const uint32_t& vCurrentFrame, bool* vOpened, ImGuiContext* vContextPt, void* vUserDatas) = 0;
    virtual bool DrawWidgets(const uint32_t& vCurrentFrame, ImGuiContext* vContextPtr, void* vUserDatas) = 0;
    virtual bool DrawOverlays(const uint32_t& vCurrentFrame, const ImRect& vRect, ImGuiContext* vContextPtr, void* vUserDatas) = 0;
    virtual bool DrawDialogsAndPopups(const uint32_t& vCurrentFrame, const ImRect& vMaxRect, ImGuiContext* vContextPtr, void* vUserDatas) = 0;

    // if for any reason the pane must be hidden temporary, the user can control this here
    virtual bool CanBeDisplayed() = 0;

    virtual void DoVirtualLayout() {}

public:
    void SetName(const LayoutPaneName& vName) {
        paneName = vName;
    }
    const LayoutPaneName& GetName() const {
        return paneName;
    }
    void SetFlag(const LayoutPaneFlag& vFlag) {
        if (paneFlag < 0) {  // ensure than this can be done only one time
            paneFlag = vFlag;
        }
    }
    const LayoutPaneFlag& GetFlag() const {
        return paneFlag;
    }
};

typedef std::weak_ptr<ILayoutPane> ILayoutPaneWeak;

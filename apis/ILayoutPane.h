/*
Copyright 2022-2026 Stephane Cuillerdier (aka aiekick)

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
#include <string>

typedef int64_t LayoutPaneFlag;  // int64_t give 64 Panes max.
typedef std::string LayoutPaneName;
typedef void* LayoutPaneUserDatas;

struct ImVec2;
struct ImRect;
struct ImGuiContext;

// project-side override of imguipack's ILayoutPane (aligned with the new interface)
class ILayoutPane {
private:
    LayoutPaneName paneName;
    LayoutPaneFlag paneFlag = -1;

public:
    virtual bool init() = 0;  // return false if the init was failed
    virtual void unit() = 0;

    // the return, is a user side use case here
    virtual bool drawPanes(bool* apOpened, LayoutPaneUserDatas apUserDatas) = 0;
    virtual bool drawWidgets(LayoutPaneUserDatas apUserDatas) = 0;
    virtual bool drawOverlays(const ImRect& aRect, LayoutPaneUserDatas apUserDatas) = 0;
    virtual bool drawDialogsAndPopups(const ImRect& aRect, LayoutPaneUserDatas apUserDatas) = 0;

    // if for any reason the pane must be hidden temporary, the user can control this here
    virtual bool canBeDisplayed() = 0;

public:
    void setName(const LayoutPaneName& aName) { paneName = aName; }
    const LayoutPaneName& getName() const { return paneName; }
    void setFlag(LayoutPaneFlag aFlag) {
        if (paneFlag < 0) {  // ensure than this can be done only one time
            paneFlag = aFlag;
        }
    }
    LayoutPaneFlag getFlag() const { return paneFlag; }
};

typedef std::weak_ptr<ILayoutPane> ILayoutPaneWeak;

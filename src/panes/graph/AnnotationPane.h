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

#include <string>
#include <vector>
#include <map>
#include <headers/DatasDef.h>
#include <ezlibs/ezClass.hpp>
#include <ezlibs/ezSingleton.hpp>
#include <imguipack.h>

class ProjectFile;
class AnnotationPane : public AbstractPane {
    DISABLE_CONSTRUCTORS(AnnotationPane)
    DISABLE_DESTRUCTORS(AnnotationPane)
    IMPLEMENT_SHARED_SINGLETON(AnnotationPane)

private:
    ImGuiListClipper m_AnnotationsListClipper;

public:
    bool init() override;
    void unit() override;
    bool drawPanes(bool* apOpened, LayoutPaneUserDatas apUserDatas) override;

private:
    void DrawContent();
    void CheckItem(SignalSeriePtr vSignalSeriePtr);
};

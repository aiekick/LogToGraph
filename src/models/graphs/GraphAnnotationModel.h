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

#include <memory>
#include <vector>
#include <headers/DatasDef.h>
#include <imguipack.h>
#include <ezlibs/ezClass.hpp>
#include <ezlibs/ezSingleton.hpp>

class GraphAnnotationModel {
    DISABLE_CONSTRUCTORS(GraphAnnotationModel)
    DISABLE_DESTRUCTORS(GraphAnnotationModel)
    IMPLEMENT_SHARED_SINGLETON(GraphAnnotationModel)

private:
    std::vector<GraphAnnotationPtr> m_GraphAnnotationModel;

public:
    // create a new annotation with the frist point and return a shared pointer
    GraphAnnotationPtr NewGraphAnnotation(const ImPlotPoint& vStartPos);

    std::vector<GraphAnnotationPtr>::iterator begin();
    std::vector<GraphAnnotationPtr>::iterator end();
    GraphAnnotationPtr& at(const size_t& vIdx);
    void erase(GraphAnnotationPtr vGraphAnnotationPtr);
    size_t size();
};

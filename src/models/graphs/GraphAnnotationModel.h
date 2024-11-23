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
#include <Headers/Globals.h>
#include <ImGuiPack.h>

class GraphAnnotationModel {
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

public:  // singleton
    static std::shared_ptr<GraphAnnotationModel> Instance() {
        static auto _instance = std::make_shared<GraphAnnotationModel>();
        return _instance;
    }

public:
    GraphAnnotationModel() = default;                                                // Prevent construction
    GraphAnnotationModel(const GraphAnnotationModel&) = delete;                      // Prevent construction by copying
    GraphAnnotationModel& operator=(const GraphAnnotationModel&) { return *this; };  // Prevent assignment
    virtual ~GraphAnnotationModel() = default;                                       // Prevent unwanted destruction};
};

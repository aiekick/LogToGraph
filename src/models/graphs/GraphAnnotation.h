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
#include <unordered_map>
#include <headers/DatasDef.h>

/*
sera rattach� a chaque graph, group� ou seuls
comme cela on pourra en creer autant qu'on veut, et il faudra un pane pour les gerer
*/

class GraphAnnotation {
public:
    static GraphAnnotationPtr Create();
    // will check if mouse pos in at less than vRadius to the line from vStart to vEnd, and return true the line nearest point vOutLinePoint
    static bool sIsMouseHoverLine(const ez::dvec2& vMousePos, const double& vRadius, const ez::dvec2& vStart, const ez::dvec2& vEnd, ez::dvec2& vOutLinePoint);
    // will check if mouse pos is at less than vRadius to the segment from vStart to vEnd, and return true the line nearest point vOutLinePoint nad the dist to line
    // vOutDistToLine
    static bool sIsMouseHoverLine2P(const ImVec2& vMousePos,
                                    const double& vRadius,
                                    const ImVec2& vStart,
                                    const ImVec2& vEnd,
                                    ImVec2& vOutLinePoint,
                                    double& vOutDistToLine);
    // will check if mouse pos is at less than vRadius to the segment from vStart to vMiddle and vMiddle to vEnd, and return true the line nearest point vOutLinePoint
    static bool sIsMouseHoverLine3P(const ImVec2& vMousePos,
                                    const double& vRadius,
                                    const ImVec2& vStart,
                                    const ImVec2& vMiddle,
                                    const ImVec2& vEnd,
                                    ImVec2& vOutLinePoint);
    // will check if mouse pos is at less than vRadius to the segment from vStart to vMiddle and vMiddle to vEnd, and return true the line nearest point vOutLinePoint
    static bool sIsMouseHoverLine4P(const ImVec2& vMousePos,
                                    const double& vRadius,
                                    const ImVec2& vp0,
                                    const ImVec2& vp1,
                                    const ImVec2& vp2,
                                    const ImVec2& vp3,
                                    ImVec2& vOutLinePoint);
    // will compute and return human readable elaspedtime
    static std::string sGetHumanReadableElapsedTime(const double& vElapsedTime);

private:  // datas
    GraphAnnotationWeak m_This;
    SignalSerieWeak m_ParentSignalSerie;
    std::string m_ElapsedTimeStr;
    ImPlotPoint m_StartPos;
    ImPlotPoint m_EndPos;
    ImPlotPoint m_LabelPos;
    ImGuiLabel m_ImGuiLabel = nullptr;
    ImVec4 m_Color;

public:
    void Clear();

    void SetStartPoint(const ImPlotPoint& vStartPoint);
    void SetEndPoint(const ImPlotPoint& vEndPoint);

    void DrawToPoint(SignalSeriePtr vSignalSeriePtr, const ImVec2& vMousePoint);
    void Draw();

    void SetSignalSerieParent(const SignalSerieWeak& vSignalSerie);
    SignalSerieWeak GetParentSignalSerie();
    ImGuiLabel GetImGuiLabel() const;

private:
    void ComputeElapsedTime();
};

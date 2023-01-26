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
#include <implot/implot.h>
#include <ctools/cTools.h>
#include <Headers/Globals.h>
#include <tinyxml2/tinyxml2.h>
#include <ctools/ConfigAbstract.h>
#include <Panes/Abstract/AbstractPane.h>

/*
sera rattaché a chaque graph, groupé ou seuls
comme cela on pourra en creer autant qu'on veut, et il faudra un pane pour les gerer
*/

class GraphAnnotation
{
public:
	static GraphAnnotationPtr Create();
	// will check if mouse pos in at less than vRadius to the line from vStart to vEnd, and return true the line nearest point vOutLinePoint
	static bool IsMouseHoverLine(const ct::dvec2& vMousePos, const double& vRadius, const ct::dvec2& vStart, const ct::dvec2& vEnd, ct::dvec2& vOutLinePoint);

private: // datas
	GraphAnnotationWeak m_This;
	SignalSerieWeak m_ParentSignalSerie;
	std::string m_ElapsedTimeStr;
	ImPlotPoint m_StartPos;
	ImPlotPoint m_EndPos;
	ImPlotPoint m_LabelPos;
	UInt8ConstPtr m_Label = nullptr;
	ImVec4 m_Color;

public:
	void Clear();

	void SetStartPoint(const ImPlotPoint& vStartPoint);
	void SetEndPoint(const ImPlotPoint& vEndPoint);

	void DrawToPoint(const ImVec2& vMousePoint);
	void Draw();

	void SetSignalSerieParent(const SignalSerieWeak& vSignalSerie);
	SignalSerieWeak GetParentSignalSerie();
	UInt8ConstPtr GetLabel() const;

private:
	void ComputeElapsedTime();

};
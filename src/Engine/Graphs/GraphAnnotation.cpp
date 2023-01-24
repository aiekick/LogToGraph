// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

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

#include "GraphAnnotation.h"

#include <Engine/Log/SignalSerie.h>
#include <Panes/Manager/LayoutManager.h>

/* code demo ees Annotation de ImPlot dans le fichier implot_demo.cpp line 1596

void Demo_Annotations() {
	static bool clamp = false;
	ImGui::Checkbox("Clamp",&clamp);
	if (ImPlot::BeginPlot("##Annotations")) {
		ImPlot::SetupAxesLimits(0,2,0,1);
		static float p[] = {0.25f, 0.25f, 0.75f, 0.75f, 0.25f};
		ImPlot::PlotScatter("##Points",&p[0],&p[1],4);
		ImVec4 col = GetLastItemColor();
		ImPlot::Annotation(0.25,0.25,col,ImVec2(-15,15),clamp,"BL");
		ImPlot::Annotation(0.75,0.25,col,ImVec2(15,15),clamp,"BR");
		ImPlot::Annotation(0.75,0.75,col,ImVec2(15,-15),clamp,"TR");
		ImPlot::Annotation(0.25,0.75,col,ImVec2(-15,-15),clamp,"TL");
		ImPlot::Annotation(0.5,0.5,col,ImVec2(0,0),clamp,"Center");

		ImPlot::Annotation(1.25,0.75,ImVec4(0,1,0,1),ImVec2(0,0),clamp);

		float bx[] = {1.2f,1.5f,1.8f};
		float by[] = {0.25f, 0.5f, 0.75f};
		ImPlot::PlotBars("##Bars",bx,by,3,0.2);
		for (int i = 0; i < 3; ++i)
			ImPlot::Annotation(bx[i],by[i],ImVec4(0,0,0,0),ImVec2(0,-5),clamp,"B[%d]=%.2f",i,by[i]);
		ImPlot::EndPlot();
	}
}
*/

void GraphAnnotation::Clear()
{
	
}
void GraphAnnotation::Draw()
{

}
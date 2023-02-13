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

#include "GraphAnnotationModel.h"
#include "GraphAnnotation.h"

GraphAnnotationPtr GraphAnnotationModel::NewGraphAnnotation(const ImPlotPoint& vStartPos)
{
	auto res = GraphAnnotation::Create();
	res->SetStartPoint(vStartPos);
	m_GraphAnnotationModel.push_back(res);
	return res;
}

std::vector<GraphAnnotationPtr>::iterator GraphAnnotationModel::begin()
{
	return m_GraphAnnotationModel.begin();
}

std::vector<GraphAnnotationPtr>::iterator GraphAnnotationModel::end()
{
	return m_GraphAnnotationModel.end();
}

GraphAnnotationPtr& GraphAnnotationModel::at(const size_t& vIdx)
{
	return m_GraphAnnotationModel.at(vIdx);
}

void GraphAnnotationModel::erase(GraphAnnotationPtr vGraphAnnotationPtr)
{
	auto item_found = std::find(m_GraphAnnotationModel.begin(), m_GraphAnnotationModel.end(), vGraphAnnotationPtr);
	if (item_found != m_GraphAnnotationModel.end())
	{
		m_GraphAnnotationModel.erase(item_found);
	}
}

size_t GraphAnnotationModel::size()
{
	return m_GraphAnnotationModel.size();
}

#include "GraphAnnotationModel.h"
#include "GraphAnnotation.h"

GraphAnnotationPtr GraphAnnotationModel::NewGraphAnnotation(const ImPlotPoint& vStartPos)
{
	auto res = GraphAnnotation::Create();
	res->SetStartPoint(vStartPos);
	m_GraphAnnotationModel.push_back(res);
	return res;
}
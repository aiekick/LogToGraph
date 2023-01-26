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

#pragma once

#include <memory>
#include <vector>
#include <ctools/cTools.h>
#include <Headers/Globals.h>
#include <implot/implot.h>

class GraphAnnotationModel
{
private:
	std::vector<GraphAnnotationPtr> m_GraphAnnotationModel;

public:
	// create a new annotation with the frist point and return a shared pointer
	GraphAnnotationPtr NewGraphAnnotation(const ImPlotPoint& vStartPos);

public: // singleton
	static std::shared_ptr<GraphAnnotationModel> Instance()
	{
		static auto _instance = std::make_shared<GraphAnnotationModel>();
		return _instance;
	}

public:
	GraphAnnotationModel() = default; // Prevent construction
	GraphAnnotationModel(const GraphAnnotationModel&) = delete; // Prevent construction by copying
	GraphAnnotationModel& operator =(const GraphAnnotationModel&) { return *this; }; // Prevent assignment
	virtual ~GraphAnnotationModel() = default; // Prevent unwanted destruction};
};
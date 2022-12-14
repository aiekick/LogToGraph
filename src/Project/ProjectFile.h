/*
 * Copyright 2020 Stephane Cuillerdier (aka Aiekick)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once

#include <string>
#include <memory>
#include <imgui/imgui.h>
#include <unordered_map>
#include <Headers/Globals.h>
#include <ctools/ConfigAbstract.h>
#include <implot/implot.h>

class ProjectFile : public conf::ConfigAbstract
{
public: // dont save
	static constexpr ImVec4 m_DefaultGraphBarColor = ImVec4(0.2f, 0.5f, 0.8f, 0.5f);
	static constexpr ImVec4 m_DefaultGraphHoveredTimeColor = ImVec4(0.8f, 0.8f, 0.2f, 0.8f);
	static constexpr ImVec4 m_DefaultGraphMouseHoveredTimeColor = ImVec4(0.2f, 0.8f, 0.2f, 0.8f);
	static constexpr ImVec4 m_DefaultGraphFirstDiffMarkColor = ImVec4(0.8f, 0.2f, 0.2f, 0.8f);
	static constexpr ImVec4 m_DefaultGraphSecondDiffMarkColor = ImVec4(0.2f, 0.2f, 0.8f, 0.8f);

public: // to save
	std::string m_ProjectFilePathName;
	std::string m_ProjectFilePath;
	ImVec4 m_GraphBarColor = m_DefaultGraphBarColor;
	ImVec4 m_GraphHoveredTimeColor = m_DefaultGraphHoveredTimeColor;
	ImVec4 m_GraphMouseHoveredTimeColor = m_DefaultGraphMouseHoveredTimeColor;
	ImVec4 m_GraphFirstDiffMarkColor = m_DefaultGraphFirstDiffMarkColor;
	ImVec4 m_GraphSecondDiffMarkColor = m_DefaultGraphSecondDiffMarkColor;
	bool m_CollapseLogSelection = false;
	std::string m_SearchString;
	std::string m_AllGraphSignalsSearchString;
	bool m_HideSomeValues = false;
	std::string m_ValuesToHide;
	bool m_AutoColorize = true; 
	uint32_t m_SignalPreview_CountX = 20U;
	float m_SignalPreview_SizeX = 20.0f;
	SignalEpochTime m_DiffFirstMark = 0.0; // first mark
	SignalEpochTime m_DiffSecondMark = 0.0; // second mark
	bool m_SyncGraphs = true;
	ImPlotRect m_SyncGraphsLimits = ImPlotRect(0, 1, 0, 1);
	std::string m_CodeFilePathName;

private: // dont save
	bool m_IsLoaded = false;
	bool m_NeverSaved = true;
	bool m_IsThereAnyNotSavedChanged = false;

public:
	void Clear();
	void New();
	void New(const std::string& vFilePathName);
	bool Load();
	bool LoadAs(const std::string vFilePathName); // ils wanted to not pass the adress for re open case
	bool Save();
	bool SaveAs(const std::string& vFilePathName);
	bool IsLoaded() const;
	bool IsNeverSaved() const;

	bool IsThereAnyNotSavedChanged() const;
	void SetProjectChange(const bool& vChange = true);

	std::string GetAbsolutePath(const std::string& vFilePathName) const;
	std::string GetRelativePath(const std::string& vFilePathName) const;

public:
	std::string getXml(const std::string& vOffset, const std::string& vUserDatas = "") override;
	bool setFromXml(tinyxml2::XMLElement* vElem, tinyxml2::XMLElement* vParent, const std::string& vUserDatas = "") override;

public: // utils
	ImVec4 GetColorFromInteger(uint32_t vInteger) const;

public: // singleton
	static ProjectFile* Instance()
	{
		static ProjectFile _instance;
		return &_instance;
	}

protected:
	ProjectFile() = default;; // Prevent construction
	ProjectFile(const ProjectFile&) {}; // Prevent construction by copying
	ProjectFile& operator =(const ProjectFile&) { return *this; }; // Prevent assignment
	~ProjectFile() = default;; // Prevent unwanted destruction};
};


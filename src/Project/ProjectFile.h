/*
 * Copyright 2022-2023 Stephane Cuillerdier (aka Aiekick)
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
#include <EzLibs/EzXmlConfig.hpp>
#include <unordered_map>
#include <Headers/Globals.h>
#include <apis/LtgPluginApi.h>

struct GraphColor
{
    ImVec4 graphBarColor = ImVec4(0.2f, 0.5f, 0.8f, 0.5f);
    ImVec4 graphHoveredTimeColor = ImVec4(0.8f, 0.8f, 0.2f, 0.8f);
    ImVec4 graphMouseHoveredTimeColor = ImVec4(0.2f, 0.8f, 0.2f, 0.8f);
    ImVec4 graphFirstDiffMarkColor = ImVec4(0.8f, 0.2f, 0.2f, 0.8f);
    ImVec4 graphSecondDiffMarkColor = ImVec4(0.2f, 0.2f, 0.8f, 0.8f);
};

class ProjectFile : public Ltg::ProjectInterface, public ez::xml::Config {
public: // to save
	GraphColor m_GraphColors;
	bool m_CollapseLogSelection = false;
	bool m_HideSomeValues = false;
	bool m_AutoColorize = true;
    bool m_SyncGraphs = true;
    std::string m_ProjectFileName;
	std::string m_ProjectFilePathName;
	std::string m_ProjectFilePath;
	std::string m_SearchString;
	std::string m_AllGraphSignalsSearchString;
	std::string m_ValuesToHide;
	std::string m_CodeFilePathName;
	std::string m_LastLogFilePath;
	uint32_t m_SignalPreview_CountX = 20U;
	float m_SignalPreview_SizeX = 20.0f;
	SignalEpochTime m_DiffFirstMark = 0.0; // first mark
	SignalEpochTime m_DiffSecondMark = 0.0; // second mark
	ImPlotRect m_SyncGraphsLimits = ImPlotRect(0, 1, 0, 1);
	double m_CurveRadiusDetection = 5.0; // for select curve for annotation
	double m_SelectedCurveDisplayThickNess = 4.0; // for display a thick curve
	double m_DefaultCurveDisplayThickNess = 2.0; // for display a default curve
	bool m_UsePredefinedZeroValue = false; // use predefined zero value
	double m_PredefinedZeroValue = 0.0; // the predefined zero value for signals

private: // dont save
    bool m_IsLoaded = false;
    bool m_NeverSaved = false;
    bool m_IsThereAnyChanges = false;
    bool m_WasJustSaved = false;
    size_t m_WasJustSavedFrameCounter = 0U;  // the state of m_WasJustSaved will be keeped during two frames

public:
    ProjectFile();
    explicit ProjectFile(const std::string& vFilePathName);
    virtual ~ProjectFile();

    void Clear();
    void ClearDatas();
    void New();
    void New(const std::string& vFilePathName);
    bool Load();
    bool LoadAs(const std::string& vFilePathName);  // ils wanted to not pass the adress for re open case
    bool Save();
    bool SaveTemporary();
    bool SaveAs(const std::string& vFilePathName);

    bool IsProjectLoaded() const override;
    bool IsProjectNeverSaved() const override;
    bool IsThereAnyProjectChanges() const override;
    void SetProjectChange(bool vChange = true) override;
    bool WasJustSaved() override;

    void NewFrame();

    std::string GetProjectFilepathName() const;

public:
    ez::xml::Nodes getXmlNodes(const std::string& vUserDatas = "") override;
    bool setFromXmlNodes(const ez::xml::Node& vNode, const ez::xml::Node& vParent, const std::string& vUserDatas) override;

public: // singleton
    static std::shared_ptr<ProjectFile> Instance() {
        static auto _instancePtr = std::make_shared<ProjectFile>();
        return _instancePtr;
    }
};


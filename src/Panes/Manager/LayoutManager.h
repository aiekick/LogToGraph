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

#include <ctools/ConfigAbstract.h>
#include <Panes/Abstract/AbstractPane.h>
#include <imgui/imgui.h>
#include <string>
#include <vector>
#include <array>
#include <map>

typedef std::string PaneCategoryName;

class ProjectFile;
class LayoutManager : public conf::ConfigAbstract
{
public:
	static bool s_PaneFlag_MenuItem(const char* label, const char* shortcut, PaneFlag* vContainer, PaneFlag vFlag, bool vOnlyOneSameTime = false)
	{
		bool selected = *vContainer & vFlag;
		const bool res = ImGui::MenuItem(label, shortcut, &selected, true);
		if (res)
		{
			if (selected)
			{
				if (vOnlyOneSameTime)
					*vContainer = vFlag; // set
				else
					*vContainer = (PaneFlag)(*vContainer | vFlag);// add
			}
			else if (!vOnlyOneSameTime)
				*vContainer = (PaneFlag)(*vContainer & ~vFlag); // remove
		}
		return res;
	}

private:
	ImGuiID m_DockSpaceID = 0;
	bool m_FirstLayout = false;
	bool m_FirstStart = true;
	std::string m_MenuLabel;
	std::string m_DefaultMenuLabel;
	std::array<float, (size_t)PaneDisposal::Count> m_PaneDisposalSizes = 
	{	0.0f, // central size is ignored because dependant of others
		200.0f, // left size
		200.0f, // right size
		200.0f, // bottom size
		200.0f // top size
	};

protected:
	std::map<PaneDisposal, AbstractPaneWeak> m_PanesByDisposal;
	std::map<std::string, AbstractPaneWeak> m_PanesByName;
	std::map<PaneCategoryName, std::vector<AbstractPaneWeak>> m_PanesInDisplayOrder;
	std::map<PaneFlag, AbstractPaneWeak> m_PanesByFlag;
	int32_t m_FlagCount = 0;

public:
	PaneFlag m_Pane_Focused_Default = 0;
	PaneFlag m_Pane_Opened_Default = 0;
	PaneFlag m_Pane_Shown = 0;
	PaneFlag m_Pane_Focused = 0;
	PaneFlag m_Pane_Hovered = 0;
	PaneFlag m_Pane_LastHovered = 0;
	ImVec2 m_LastSize;

public:
	PaneFlag m_LastVirtualPaneFlag = 0; // virtual mean can be created dynamically

public:
	void AddPane(
		const AbstractPaneWeak& vPane,
		const std::string& vName,
		const PaneCategoryName& vCategory,
		const PaneDisposal& vPaneDisposal,
		const bool& vIsOpenedDefault,
		const bool& vIsFocusedDefault);
	void AddPane(
		const AbstractPaneWeak& vPane,
		const std::string& vName,
		const PaneCategoryName& vCategory,
		const PaneFlag& vFlag,
		const PaneDisposal& vPaneDisposal,
		const bool& vIsOpenedDefault,
		const bool& vIsFocusedDefault);
	void SetPaneDisposalSize(const PaneDisposal& vPaneDisposal, const float& vSize);

public:
	void Init(const std::string& vMenuLabel, const std::string& vDefaultMenuLabel);
	void Unit();

	bool InitPanes();
	void UnitPanes();

	void InitAfterFirstDisplay(const ImVec2& vSize);
	bool BeginDockSpace(const ImGuiDockNodeFlags& vFlags);
	void EndDockSpace();
	bool IsDockSpaceHoleHovered();

	void ApplyInitialDockingLayout(const ImVec2& vSize = ImVec2(0, 0));

	virtual void DisplayMenu(const ImVec2& vSize);
	virtual int DisplayPanes(const uint32_t& vCurrentFrame, const int& vWidgetId, const std::string& vUserDatas);
	virtual void DrawDialogsAndPopups(const uint32_t& vCurrentFrame, const std::string& vUserDatas);
	virtual int DrawWidgets(const uint32_t& vCurrentFrame, const int& vWidgetId, const std::string& vUserDatas);

public: // virtual pane flags
	uint32_t GetRegisteredPaneCount();
	uint32_t GetMaxPossiblePaneCount();
	bool IncVirtualPaneFlag(PaneFlag& vOutPaneFlag);
	void DecVirtualPaneFlag();

public:
	void ShowSpecificPane(const PaneFlag& vPane);
	void HideSpecificPane(const PaneFlag& vPane);
	void FocusSpecificPane(const PaneFlag& vPane);
	void FocusSpecificPane(const std::string& vlabel);
	void ShowAndFocusSpecificPane(const PaneFlag& vPane);
	bool IsSpecificPaneFocused(const PaneFlag& vPane);
	bool IsSpecificPaneFocused(const std::string& vlabel);
	void AddSpecificPaneToExisting(const std::string& vNewPane, const std::string& vExistingPane);

private: // configuration
	PaneFlag Internal_GetFocusedPanes();
	void Internal_SetFocusedPanes(const PaneFlag& vActivePanes);

public: // configuration
	std::string getXml(const std::string& vOffset, const std::string& vUserDatas);
	bool setFromXml(tinyxml2::XMLElement* vElem, tinyxml2::XMLElement* vParent, const std::string& vUserDatas);

public: // singleton
	static LayoutManager *Instance(LayoutManager* vCopy = nullptr, bool vForce = false)
	{
		static LayoutManager _instance;
		static LayoutManager* _instance_copy = nullptr;
		if (vCopy || vForce)
		{
			_instance_copy = vCopy;
		}
		if (_instance_copy)
		{
			return _instance_copy;
		}
		return &_instance;
	}

protected:
	LayoutManager(); // Prevent construction
	LayoutManager(const LayoutManager&) = delete;
	LayoutManager& operator =(const LayoutManager&) = delete;
	virtual ~LayoutManager(); // Prevent unwanted destruction
};


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

#include <string>
#include <vector>
#include <map>
#include <ImGuiPack.h>

class ProjectFile;
class GraphPane : public AbstractPane
{
private:
	bool m_show_hide_x_axis = true;
	bool m_show_hide_y_axis = false;

public:
	bool Init() override;
    void Unit() override;
    bool DrawPanes(const uint32_t& vCurrentFrame, bool* vOpened = nullptr, ImGuiContext* vContextPtr = nullptr, void* vUserDatas = nullptr) override;
	void DoVirtualLayout() override;

public: // singleton
	static std::shared_ptr<GraphPane> Instance()
	{
		static auto _instance = std::make_shared<GraphPane>();
		return _instance;
	}

public:
	GraphPane() = default; // Prevent construction
	GraphPane(const GraphPane&) = delete; // Prevent construction by copying
	GraphPane& operator =(const GraphPane&) { return *this; }; // Prevent assignment
    virtual ~GraphPane() = default; // Prevent unwanted destruction};
};


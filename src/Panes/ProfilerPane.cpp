// NoodlesPlate Copyright (C) 2017-2023 Stephane Cuillerdier aka Aiekick
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include "ProfilerPane.h"

#include <ImGuiPack.h>
#include <iagp.h>

#include <cinttypes>  // printf zu

ProfilerPane::ProfilerPane() = default;
ProfilerPane::~ProfilerPane() = default;

///////////////////////////////////////////////////////////////////////////////////
//// OVERRIDES ////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////

bool ProfilerPane::Init() {
    return true;
}

void ProfilerPane::Unit() {
}

bool ProfilerPane::DrawPanes(const uint32_t& vCurrentFrame, bool* vOpened, ImGuiContext* vContextPtr, void* vUserDatas) {
    iagp::InAppGpuProfiler::Instance()->sIsActive = false;

    if (vOpened != nullptr && *vOpened) {
        iagp::InAppGpuProfiler::Instance()->sIsActive = true;  // is opened but can be invisible if repalce but another windows like a child flame graph

        static ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_MenuBar;
        if (ImGui::Begin(GetName().c_str(), vOpened, flags)) {
#ifdef USE_DECORATIONS_FOR_RESIZE_CHILD_WINDOWS
            auto win = ImGui::GetCurrentWindowRead();
            if (win->Viewport->Idx != 0)
                flags |= ImGuiWindowFlags_NoResize;  // | ImGuiWindowFlags_NoTitleBar;
            else
                flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_MenuBar;
#endif
            // draw iagp frame
            iagp::InAppGpuProfiler::Instance()->DrawFlamGraphNoWin();
        }

        // MainFrame::sAnyWindowsHovered |= ImGui::IsWindowHovered();

        ImGui::End();

        iagp::InAppGpuProfiler::Instance()->DrawFlamGraphChilds();

        iagp::InAppGpuProfiler::Instance()->DrawDetails();
    }

    return false;
}

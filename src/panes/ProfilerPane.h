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

#pragma once

#include <ezlibs/ezClass.hpp>
#include <ezlibs/ezSingleton.hpp>
#include <imguipack.h>
#include <ImGuiFileDialog.h>

#include <string>

class ProjectFile;
class ProfilerPane : public AbstractPane {
    DISABLE_CONSTRUCTORS(ProfilerPane)
    DISABLE_DESTRUCTORS(ProfilerPane)
    IMPLEMENT_SHARED_SINGLETON(ProfilerPane)

private:
    LayoutPaneFlag m_InOutPaneShown = -1;

public:
    bool init() override;
    void unit() override;
    bool drawPanes(bool* apOpened, LayoutPaneUserDatas apUserDatas) override;
};

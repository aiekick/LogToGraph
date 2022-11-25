// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include "FrameActionSystem.h"
#include <Project/ProjectFile.h>

void FrameActionSystem::Insert(ActionStamp vAction)
{
	if (vAction)
		puActions.push_front(vAction);
}

void FrameActionSystem::Add(ActionStamp vAction)
{
	if (vAction)
		puActions.push_back(vAction);
}

void FrameActionSystem::Clear()
{
	puActions.clear();
}

void FrameActionSystem::RunActions()
{
	if (!puActions.empty())
	{
		const auto action = *puActions.begin();
		if (action()) // one action per frame, it true we can continue by deleting the current
		{
			if (!puActions.empty()) // because an action can clear actions
			{
				puActions.pop_front();
			}
		}
	}
}
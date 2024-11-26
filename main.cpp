// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

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

#include <App.h>
#include <string>
#include <iostream>

#include <ezlibs/ezLog.hpp>
#include <ezlibs/ezTools.hpp>

int main(int argc, char** argv) {
#ifdef _MSC_VER
#ifdef _DEBUG
    // active memory leak detector
    // https://stackoverflow.com/questions/4790564/finding-memory-leaks-in-a-c-application-with-visual-studio
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF | _CRTDBG_CHECK_CRT_DF);
    _CrtMemState sOld;
    _CrtMemCheckpoint(&sOld);  // take a snapshot
#endif
#endif

    try {
        App app(argc, argv);
        app.run();
        ez::Log::instance()->close();
    } catch (const std::exception& e) {
        LogVarLightInfo("Exception %s", e.what());
        ez::Log::instance()->close();
        EZ_TOOLS_DEBUG_BREAK;
        return EXIT_FAILURE;
    }

#ifdef _MSC_VER
#ifdef _DEBUG
    _CrtMemState sNew;
    _CrtMemCheckpoint(&sNew);  // take a snapshot
    _CrtMemState sDiff;
    if (_CrtMemDifference(&sDiff, &sOld, &sNew)) {  // if there is a difference
        std::cout << "-----------_CrtMemDumpStatistics ---------" << std::endl;
        _CrtMemDumpStatistics(&sDiff);
        std::cout << "-----------_CrtMemDumpAllObjectsSince ---------" << std::endl;
        _CrtMemDumpAllObjectsSince(&sOld);
        std::cout << "-----------_CrtDumpMemoryLeaks ---------" << std::endl;
        _CrtDumpMemoryLeaks();
    }
#endif
#endif

    return EXIT_SUCCESS;
}

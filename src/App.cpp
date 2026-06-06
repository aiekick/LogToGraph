// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include "App.h"

#include <headers/LogToGraphBuild.h>
#include <backend/MainBackend.h>
#include <frontend/MainFrontend.h>
#include <project/ProjectFile.h>
#include <systems/PluginManager.h>
#include <systems/SettingsDialog.h>
#include <systems/AppSettings.h>
#include <systems/TranslationHelper.h>
#include <models/log/LogEngine.h>
#include <models/script/ScriptingEngine.h>
#include <models/database/DataBase.h>
#include <models/graphs/GraphView.h>
#include <models/graphs/GraphAnnotationModel.h>
#include <models/debug/ScriptDebugger.h>

#include <panes/CodePane.h>
#include <panes/ConsolePane.h>
#include <panes/ProfilerPane.h>
#include <panes/AnnotationPane.h>
#include <panes/LogPane.h>
#include <panes/LogPaneSecondView.h>
#include <panes/GraphPane.h>
#include <panes/GraphListPane.h>
#include <panes/GraphGroupPane.h>
#include <panes/SignalsHoveredList.h>
#include <panes/SignalsHoveredDiff.h>
#include <panes/SignalsHoveredMap.h>
#include <panes/SignalsPreview.h>
#include <panes/ToolPane.h>
#include <panes/StackTreePane.h>
#include <panes/ScopePane.h>
#include <panes/CalltracePane.h>
#include <panes/BreakpointsPane.h>

#include <imguipack.h>
#include <iagp.h>
#include <ezlibs/ezLog.hpp>

// messaging
#define MESSAGING_CODE_INFOS 0
#define MESSAGING_LABEL_INFOS "Infos"
#define MESSAGING_CODE_WARNINGS 1
#define MESSAGING_LABEL_WARNINGS "Warnings"
#define MESSAGING_CODE_ERRORS 2
#define MESSAGING_CODE_DEBUG 3
#define MESSAGING_LABEL_ERRORS "Errors"
#define MESSAGING_LABEL_DEBUG "Debug"

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////

App::App(int aArgc, char** apArgv) : ez::App(aArgc, apArgv) {}

int App::run() {
    m_InitSingletons();
    m_InitMessaging();

    LogVarLightInfo("-----------");
    LogVarLightInfo("[[ %s Beta %s ]]", LogToGraph_Prefix, LogToGraph_BuildId);

    MainBackend::ref().run(getAppPath());

    m_UnitSingletons();
    return 0;
}

void App::m_InitMessaging() {
    Messaging::ref().AddCategory(MESSAGING_CODE_INFOS, "Infos(s)", MESSAGING_LABEL_INFOS, ImVec4(0.0f, 0.8f, 0.0f, 1.0f));
    Messaging::ref().AddCategory(MESSAGING_CODE_WARNINGS, "Warnings(s)", MESSAGING_LABEL_WARNINGS, ImVec4(0.8f, 0.8f, 0.0f, 1.0f));
    Messaging::ref().AddCategory(MESSAGING_CODE_ERRORS, "Errors(s)", MESSAGING_LABEL_ERRORS, ImVec4(0.8f, 0.0f, 0.0f, 1.0f));
    Messaging::ref().AddCategory(MESSAGING_CODE_DEBUG, "Debug(s)", MESSAGING_LABEL_DEBUG, ImVec4(0.8f, 0.8f, 0.0f, 1.0f));
    Messaging::ref().SetImLayout(&ImLayout::ref());
    ez::Log::ref().setStandardLogMessageFunctor([](const int& vType, const std::string& vMessage) {
        MessageData msg_datas;
        const auto& type = vType;
        Messaging::ref().AddMessage(vMessage, type, false, msg_datas, {});
    });
}

void App::m_InitSingletons() {
    // imguipack / iagp singletons — ImLayout::ref() crashes if initSingleton() was not
    // called first (it does NOT lazy-init the way Messaging/ImGuiThemeHelper/etc. do)
    ImLayout::initSingleton();
    Messaging::initSingleton();
    ImGuiThemeHelper::initSingleton();
    ImGuiFileDialog::initSingleton();
    iagp::InAppGpuProfiler::initSingleton();
    // managers (unique_ptr based)
    MainBackend::initSingleton();
    MainFrontend::initSingleton();
    PluginManager::initSingleton();
    SettingsDialog::initSingleton();
    TranslationHelper::initSingleton();
    // app-level settings — shared_ptr because SettingsDialog stores it as weak_ptr<ISettings>
    AppSettings::initSingleton();
    // models (shared_ptr based)
    ProjectFile::initSingleton();
    LogEngine::initSingleton();
    ScriptingEngine::initSingleton();
    DataBase::initSingleton();
    GraphView::initSingleton();
    GraphAnnotationModel::initSingleton();
    ScriptDebugger::initSingleton();
    // panes (shared_ptr based)
    CodePane::initSingleton();
    ConsolePane::initSingleton();
    ProfilerPane::initSingleton();
    AnnotationPane::initSingleton();
    LogPane::initSingleton();
    LogPaneSecondView::initSingleton();
    GraphPane::initSingleton();
    GraphListPane::initSingleton();
    GraphGroupPane::initSingleton();
    SignalsHoveredList::initSingleton();
    SignalsHoveredDiff::initSingleton();
    SignalsHoveredMap::initSingleton();
    SignalsPreview::initSingleton();
    ToolPane::initSingleton();
    StackTreePane::initSingleton();
    ScopePane::initSingleton();
    CalltracePane::initSingleton();
    BreakpointsPane::initSingleton();
}

void App::m_UnitSingletons() {
    // stop a possibly running/paused parsing worker BEFORE destroying the singletons it uses:
    // a worker blocked in ScriptDebugger::onPause would otherwise outlive ScriptDebugger
    // (use-after-free), and std::thread's dtor would std::terminate on a still-joinable thread.
    ScriptingEngine::ref()->AbortAndJoinWorker();
    // panes
    BreakpointsPane::unitSingleton();
    CalltracePane::unitSingleton();
    ScopePane::unitSingleton();
    StackTreePane::unitSingleton();
    ToolPane::unitSingleton();
    SignalsPreview::unitSingleton();
    SignalsHoveredMap::unitSingleton();
    SignalsHoveredDiff::unitSingleton();
    SignalsHoveredList::unitSingleton();
    GraphGroupPane::unitSingleton();
    GraphListPane::unitSingleton();
    GraphPane::unitSingleton();
    LogPaneSecondView::unitSingleton();
    LogPane::unitSingleton();
    AnnotationPane::unitSingleton();
    ProfilerPane::unitSingleton();
    ConsolePane::unitSingleton();
    CodePane::unitSingleton();
    // models
    ScriptDebugger::unitSingleton();
    GraphAnnotationModel::unitSingleton();
    GraphView::unitSingleton();
    DataBase::unitSingleton();
    ScriptingEngine::unitSingleton();
    LogEngine::unitSingleton();
    ProjectFile::unitSingleton();
    // app-level settings
    AppSettings::unitSingleton();
    // managers
    TranslationHelper::unitSingleton();
    SettingsDialog::unitSingleton();
    PluginManager::unitSingleton();
    MainFrontend::unitSingleton();
    MainBackend::unitSingleton();
    // imguipack / iagp singletons
    iagp::InAppGpuProfiler::unitSingleton();
    ImGuiFileDialog::unitSingleton();
    ImGuiThemeHelper::unitSingleton();
    Messaging::unitSingleton();
    ImLayout::unitSingleton();
}

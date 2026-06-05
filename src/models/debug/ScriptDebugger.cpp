/*
Copyright 2022-2026 Stephane Cuillerdier (aka aiekick)

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

#include "ScriptDebugger.h"

///////////////////////////////////////////////////////////////////////////////////
//// RENDEZVOUS (worker thread) /////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////

Ltg::DebugCommand ScriptDebugger::onPause(const Ltg::DebugState& aState) {
    std::unique_lock<std::mutex> lock(m_Mutex);
    m_State = aState;
    m_Mode = Mode::Paused;
    m_HasCommand = false;
    m_Cond.wait(lock, [this]() { return m_HasCommand; });
    m_HasCommand = false;
    m_Mode = Mode::Running;
    return m_Command;
}

///////////////////////////////////////////////////////////////////////////////////
//// SESSION BINDING ////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////

void ScriptDebugger::bindPlugin(Ltg::IScriptDebugger* apPluginDebugger) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_PluginDebuggerPtr = apPluginDebugger;
    m_Mode = Mode::Running;
    m_HasCommand = false;
}

void ScriptDebugger::unbindPlugin() {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_PluginDebuggerPtr = nullptr;
    m_Mode = Mode::Idle;
}

bool ScriptDebugger::shouldArmDebug() const {
    std::lock_guard<std::mutex> lock(m_BreakpointsMutex);
    return m_DebugArmed || !m_Breakpoints.empty();
}

Ltg::BreakpointLines ScriptDebugger::getBreakpoints() const {
    std::lock_guard<std::mutex> lock(m_BreakpointsMutex);
    return m_Breakpoints;
}

///////////////////////////////////////////////////////////////////////////////////
//// BREAKPOINTS (UI thread) ////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////

void ScriptDebugger::setBreakpoint(const std::string& aScriptFilePathName, int32_t aLine, bool aAdd) {
    std::lock_guard<std::mutex> lock(m_BreakpointsMutex);
    m_ScriptFilePathName = aScriptFilePathName;
    if (aAdd) {
        m_Breakpoints.insert(aLine);
    } else {
        m_Breakpoints.erase(aLine);
    }
}

void ScriptDebugger::toggleBreakpoint(const std::string& aScriptFilePathName, int32_t aLine) {
    std::lock_guard<std::mutex> lock(m_BreakpointsMutex);
    m_ScriptFilePathName = aScriptFilePathName;
    if (m_Breakpoints.find(aLine) != m_Breakpoints.end()) {
        m_Breakpoints.erase(aLine);
    } else {
        m_Breakpoints.insert(aLine);
    }
}

void ScriptDebugger::clearBreakpoints() {
    std::lock_guard<std::mutex> lock(m_BreakpointsMutex);
    m_Breakpoints.clear();
}

std::string ScriptDebugger::getScriptFilePathName() const {
    std::lock_guard<std::mutex> lock(m_BreakpointsMutex);
    return m_ScriptFilePathName;
}

void ScriptDebugger::setDebugArmed(bool aArmed) {
    std::lock_guard<std::mutex> lock(m_BreakpointsMutex);
    m_DebugArmed = aArmed;
}

bool ScriptDebugger::isDebugArmed() const {
    std::lock_guard<std::mutex> lock(m_BreakpointsMutex);
    return m_DebugArmed;
}

///////////////////////////////////////////////////////////////////////////////////
//// EXECUTION CONTROL (UI thread) //////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////

ScriptDebugger::Mode ScriptDebugger::getMode() const {
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_Mode;
}

Ltg::DebugState ScriptDebugger::getState() const {
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_State;
}

void ScriptDebugger::doContinue() {
    m_pushCommand(Ltg::DebugCommand::Continue);
}

void ScriptDebugger::stepInto() {
    m_pushCommand(Ltg::DebugCommand::StepInto);
}

void ScriptDebugger::stepOver() {
    m_pushCommand(Ltg::DebugCommand::StepOver);
}

void ScriptDebugger::stepOut() {
    m_pushCommand(Ltg::DebugCommand::StepOut);
}

void ScriptDebugger::pause() {
    std::lock_guard<std::mutex> lock(m_Mutex);
    if (m_PluginDebuggerPtr != nullptr) {
        m_PluginDebuggerPtr->requestPause();
    }
}

void ScriptDebugger::stop() {
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        if (m_PluginDebuggerPtr != nullptr) {
            m_PluginDebuggerPtr->requestStop();
        }
    }
    // also unblock a possible pending onPause so the worker can abort right away
    m_pushCommand(Ltg::DebugCommand::Stop);
}

void ScriptDebugger::m_pushCommand(Ltg::DebugCommand aCommand) {
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        m_Command = aCommand;
        m_HasCommand = true;
    }
    m_Cond.notify_one();
}

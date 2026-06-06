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

Ltg::DebugAction ScriptDebugger::onPause(const Ltg::DebugState& aState) {
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        m_State = aState;
        m_StateRevision.fetch_add(1, std::memory_order_release);
        m_Mode.store(Mode::Paused, std::memory_order_release);
    }
    {
        std::lock_guard<std::mutex> treeLock(m_TreeMutex);
        m_Expansions.clear();    // a fresh pause invalidates every previously read ref
        m_EvalResults.clear();   // and every previously evaluated watch expression
    }
    return m_waitNextAction();
}

Ltg::DebugAction ScriptDebugger::waitAction() {
    return m_waitNextAction();
}

void ScriptDebugger::publishExpansion(int32_t aRef, const std::vector<Ltg::DebugVar>& aChildren) {
    std::lock_guard<std::mutex> treeLock(m_TreeMutex);
    m_Expansions[aRef] = aChildren;  // cached until the next pause; getChildren stops the re-posting
}

void ScriptDebugger::publishEvalResult(int32_t aEvalId, const Ltg::EvalResult& aResult) {
    std::lock_guard<std::mutex> treeLock(m_TreeMutex);
    m_EvalResults[aEvalId] = aResult;  // cached until the next pause; getEvalResult stops the re-posting
}

///////////////////////////////////////////////////////////////////////////////////
//// SESSION BINDING ////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////

void ScriptDebugger::bindPlugin(Ltg::IScriptDebugger* apPluginDebugger) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_PluginDebuggerPtr = apPluginDebugger;
    m_Mode.store(Mode::Running, std::memory_order_release);
    m_HasCommand = false;
    m_HasExpand = false;
    m_HasEval = false;
}

void ScriptDebugger::unbindPlugin() {
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        m_PluginDebuggerPtr = nullptr;
        m_Mode.store(Mode::Idle, std::memory_order_release);
    }
    std::lock_guard<std::mutex> treeLock(m_TreeMutex);
    m_Expansions.clear();
    m_EvalResults.clear();
}

bool ScriptDebugger::shouldArmDebug() const {
    std::lock_guard<std::mutex> lock(m_BreakpointsMutex);
    // armed is atomic; the lock here is for m_Breakpoints. relaxed is enough — the lock provides ordering.
    return m_DebugArmed.load(std::memory_order_relaxed) || !m_Breakpoints.empty();
}

const Ltg::BreakpointLines& ScriptDebugger::getBreakpoints() const {
    return m_Breakpoints;
}

int64_t ScriptDebugger::getBreakpointsRevision() const {
    return m_BreakpointsRevision.load(std::memory_order_acquire);
}

bool ScriptDebugger::isBreakpoint(int32_t aLine) {
    std::lock_guard<std::mutex> lock(m_BreakpointsMutex);
    return m_Breakpoints.find(aLine) != m_Breakpoints.end();
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
    m_BreakpointsRevision.fetch_add(1, std::memory_order_release);
}

void ScriptDebugger::toggleBreakpoint(const std::string& aScriptFilePathName, int32_t aLine) {
    std::lock_guard<std::mutex> lock(m_BreakpointsMutex);
    m_ScriptFilePathName = aScriptFilePathName;
    if (m_Breakpoints.find(aLine) != m_Breakpoints.end()) {
        m_Breakpoints.erase(aLine);
    } else {
        m_Breakpoints.insert(aLine);
    }
    m_BreakpointsRevision.fetch_add(1, std::memory_order_release);
}

void ScriptDebugger::clearBreakpoints() {
    std::lock_guard<std::mutex> lock(m_BreakpointsMutex);
    m_Breakpoints.clear();
    m_BreakpointsRevision.fetch_add(1, std::memory_order_release);
}

void ScriptDebugger::Clear() {
    // disarm first (lock-free atomic), then wipe breakpoints+path + paused snapshot under their own locks
    m_DebugArmed.store(false, std::memory_order_release);
    {
        std::lock_guard<std::mutex> lock(m_BreakpointsMutex);
        m_Breakpoints.clear();
        m_ScriptFilePathName.clear();
        m_BreakpointsRevision.fetch_add(1, std::memory_order_release);
    }
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        m_State = Ltg::DebugState{};
        m_StateRevision.fetch_add(1, std::memory_order_release);
    }
    {
        std::lock_guard<std::mutex> treeLock(m_TreeMutex);
        m_Expansions.clear();
        m_EvalResults.clear();
    }
}

const std::string& ScriptDebugger::getScriptFilePathName() const {
    return m_ScriptFilePathName;
}

void ScriptDebugger::setDebugArmed(bool aArmed) {
    m_DebugArmed.store(aArmed, std::memory_order_release);
}

bool ScriptDebugger::isDebugArmed() const {
    return m_DebugArmed.load(std::memory_order_acquire);
}

///////////////////////////////////////////////////////////////////////////////////
//// EXECUTION CONTROL (UI thread) //////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////

ScriptDebugger::Mode ScriptDebugger::getMode() const {
    return m_Mode.load(std::memory_order_acquire);
}

Ltg::DebugState ScriptDebugger::getState() const {
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_State;
}

int64_t ScriptDebugger::getStateRevision() const {
    return m_StateRevision.load(std::memory_order_acquire);
}

void ScriptDebugger::doContinue() {
    m_setCommand(Ltg::DebugCommand::Continue);
}

void ScriptDebugger::stepInto() {
    m_setCommand(Ltg::DebugCommand::StepInto);
}

void ScriptDebugger::stepOver() {
    m_setCommand(Ltg::DebugCommand::StepOver);
}

void ScriptDebugger::stepOut() {
    m_setCommand(Ltg::DebugCommand::StepOut);
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
    // also unblock a possible pending wait so the worker aborts right away
    m_setCommand(Ltg::DebugCommand::Stop);
}

///////////////////////////////////////////////////////////////////////////////////
//// LAZY EXPANSION (UI thread) /////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////

void ScriptDebugger::requestExpand(int32_t aRef) {
    if (aRef < 0) {
        return;
    }
    {
        std::lock_guard<std::mutex> treeLock(m_TreeMutex);
        if (m_Expansions.find(aRef) != m_Expansions.end()) {
            return;  // already fetched
        }
    }
    // not cached yet: (re)post the request on the expansion channel. it never overwrites a
    // pending command, and the slot drains one node per frame (resolves over a few frames).
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        m_ExpandRef = aRef;
        m_HasExpand = true;
    }
    m_Cond.notify_one();
}

bool ScriptDebugger::getChildren(int32_t aRef, std::vector<Ltg::DebugVar>& aoChildren) const {
    std::lock_guard<std::mutex> treeLock(m_TreeMutex);
    const auto it = m_Expansions.find(aRef);
    if (it == m_Expansions.end()) {
        return false;
    }
    aoChildren = it->second;
    return true;
}

void ScriptDebugger::requestEval(int32_t aEvalId, const std::string& aExpression) {
    if (aEvalId < 0 || aExpression.empty()) {
        return;
    }
    {
        std::lock_guard<std::mutex> treeLock(m_TreeMutex);
        if (m_EvalResults.find(aEvalId) != m_EvalResults.end()) {
            return;  // already evaluated since the current pause
        }
    }
    // not cached yet: (re)post on the eval channel. eval is overwriteable by a control command,
    // so the user can still resume/step while a long expression is being evaluated.
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        m_EvalId = aEvalId;
        m_EvalExpression = aExpression;
        m_HasEval = true;
    }
    m_Cond.notify_one();
}

bool ScriptDebugger::getEvalResult(int32_t aEvalId, Ltg::EvalResult& aoResult) const {
    std::lock_guard<std::mutex> treeLock(m_TreeMutex);
    const auto it = m_EvalResults.find(aEvalId);
    if (it == m_EvalResults.end()) {
        return false;
    }
    aoResult = it->second;
    return true;
}

void ScriptDebugger::m_setCommand(Ltg::DebugCommand aCommand) {
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        m_Command = aCommand;
        m_HasCommand = true;
    }
    m_Cond.notify_one();
}

Ltg::DebugAction ScriptDebugger::m_waitNextAction() {
    std::unique_lock<std::mutex> lock(m_Mutex);
    m_Cond.wait(lock, [this]() { return m_HasCommand || m_HasEval || m_HasExpand; });
    Ltg::DebugAction action;
    if (m_HasCommand) {  // control command has top priority — supersedes pending eval/expand
        m_HasCommand = false;
        m_HasEval = false;
        m_HasExpand = false;
        action.kind = Ltg::DebugAction::Kind::Command;
        action.command = m_Command;
        m_Mode.store(Mode::Running, std::memory_order_release);
    } else if (m_HasEval) {
        m_HasEval = false;
        action.kind = Ltg::DebugAction::Kind::Eval;
        action.evalId = m_EvalId;
        action.evalExpression = m_EvalExpression;
    } else {
        m_HasExpand = false;
        action.kind = Ltg::DebugAction::Kind::Expand;
        action.expandRef = m_ExpandRef;
    }
    return action;
}

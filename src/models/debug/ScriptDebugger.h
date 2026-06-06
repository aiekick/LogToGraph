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

#pragma once

#include <mutex>
#include <string>
#include <vector>
#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <condition_variable>

#include <apis/IScriptDebugger.h>
#include <ezlibs/ezClass.hpp>
#include <ezlibs/ezSingleton.hpp>

// Host side of the common debugger: single source of truth for breakpoints, the
// paused snapshot and the lazy-expansion cache, and the rendezvous the plugin
// blocks on (IScriptDebugHost). All scripting plugins drive the same instance.
class ScriptDebugger : public Ltg::IScriptDebugHost {
    DISABLE_CONSTRUCTORS(ScriptDebugger)
    DISABLE_DESTRUCTORS(ScriptDebugger)
    IMPLEMENT_SHARED_SINGLETON(ScriptDebugger)

public:
    enum class Mode { Idle, Running, Paused };

private:  // rendezvous between the worker thread (onPause/waitAction) and the UI thread
    mutable std::mutex m_Mutex;
    std::condition_variable m_Cond;
    Mode m_Mode{Mode::Idle};
    bool m_HasCommand{false};  // control command (continue/step/stop) — has priority
    Ltg::DebugCommand m_Command{Ltg::DebugCommand::Continue};
    bool m_HasExpand{false};  // pending lazy-expansion request
    int32_t m_ExpandRef{-1};
    Ltg::DebugState m_State;
    Ltg::IScriptDebugger* m_PluginDebuggerPtr{nullptr};

private:  // lazy expansion cache (children read by the worker, displayed by the UI)
    mutable std::mutex m_TreeMutex;
    std::unordered_map<int32_t, std::vector<Ltg::DebugVar>> m_Expansions;

private:  // breakpoints, source of truth, lines are 1-based
    mutable std::mutex m_BreakpointsMutex;
    Ltg::BreakpointLines m_Breakpoints;
    std::string m_ScriptFilePathName;
    bool m_DebugArmed{false};

public:
    // IScriptDebugHost — called by the plugin from the worker thread
    Ltg::DebugAction onPause(const Ltg::DebugState& aState) final;
    Ltg::DebugAction waitAction() final;
    void publishExpansion(int32_t aRef, const std::vector<Ltg::DebugVar>& aChildren) final;
    bool isBreakpoint(int32_t aLine) final;

    // session binding — called by ScriptingEngine around the parse run
    void bindPlugin(Ltg::IScriptDebugger* apPluginDebugger);
    void unbindPlugin();
    bool shouldArmDebug() const;
    Ltg::BreakpointLines getBreakpoints() const;

    // breakpoints — UI thread, lines are 1-based
    void setBreakpoint(const std::string& aScriptFilePathName, int32_t aLine, bool aAdd);
    void toggleBreakpoint(const std::string& aScriptFilePathName, int32_t aLine);
    void clearBreakpoints();
    std::string getScriptFilePathName() const;

    // debug arming — UI thread
    void setDebugArmed(bool aArmed);
    bool isDebugArmed() const;

    // execution control — UI thread
    Mode getMode() const;
    Ltg::DebugState getState() const;
    void doContinue();
    void stepInto();
    void stepOver();
    void stepOut();
    void pause();
    void stop();

    // lazy node expansion — UI thread
    void requestExpand(int32_t aRef);
    bool getChildren(int32_t aRef, std::vector<Ltg::DebugVar>& aoChildren) const;

private:
    void m_setCommand(Ltg::DebugCommand aCommand);
    Ltg::DebugAction m_waitNextAction();
};

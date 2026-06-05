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
#include <cstdint>
#include <condition_variable>

#include <apis/IScriptDebugger.h>
#include <ezlibs/ezClass.hpp>
#include <ezlibs/ezSingleton.hpp>

// Host side of the common debugger. It is the single source of truth for the
// breakpoint set and the last paused snapshot, and it implements the rendezvous
// the plugin blocks on (IScriptDebugHost::onPause). All scripting plugins drive
// the same instance.
class ScriptDebugger : public Ltg::IScriptDebugHost {
    DISABLE_CONSTRUCTORS(ScriptDebugger)
    DISABLE_DESTRUCTORS(ScriptDebugger)
    IMPLEMENT_SHARED_SINGLETON(ScriptDebugger)

public:
    enum class Mode { Idle, Running, Paused };

private:  // rendezvous between the worker thread (onPause) and the UI thread (commands)
    mutable std::mutex m_Mutex;
    std::condition_variable m_Cond;
    Mode m_Mode{Mode::Idle};
    bool m_HasCommand{false};
    Ltg::DebugCommand m_Command{Ltg::DebugCommand::Continue};
    Ltg::DebugState m_State;
    // active plugin debugger during a session, non-owning weak handle
    Ltg::IScriptDebugger* m_PluginDebuggerPtr{nullptr};

private:  // breakpoints, source of truth, lines are 1-based
    mutable std::mutex m_BreakpointsMutex;
    Ltg::BreakpointLines m_Breakpoints;
    std::string m_ScriptFilePathName;
    bool m_DebugArmed{false};

public:
    // IScriptDebugHost — called by the plugin from the worker thread, blocks until a command
    Ltg::DebugCommand onPause(const Ltg::DebugState& aState) final;

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

private:
    void m_pushCommand(Ltg::DebugCommand aCommand);
};

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
#include <atomic>
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
    // atomic so getMode() is lock-free; writers still set it inside m_Mutex (paired with m_HasCommand etc.).
    std::atomic<Mode> m_Mode{Mode::Idle};
    bool m_HasCommand{false};  // control command (continue/step/stop) — has priority
    Ltg::DebugCommand m_Command{Ltg::DebugCommand::Continue};
    bool m_HasEval{false};  // pending watch-eval request — fires between commands and expansions
    int32_t m_EvalId{-1};
    std::string m_EvalExpression;
    bool m_HasExpand{false};  // pending lazy-expansion request
    int32_t m_ExpandRef{-1};
    Ltg::DebugState m_State;
    // monotonic counter bumped (release) in onPause after m_State assignment. UI panes cache the
    // state by revision and skip the mutex+copy of getState() between pauses.
    std::atomic<int64_t> m_StateRevision{0};
    Ltg::IScriptDebugger* m_PluginDebuggerPtr{nullptr};

private:  // lazy expansion + eval caches (children/values read by the worker, displayed by the UI)
    mutable std::mutex m_TreeMutex;
    std::unordered_map<int32_t, std::vector<Ltg::DebugVar>> m_Expansions;
    std::unordered_map<int32_t, Ltg::EvalResult> m_EvalResults;

private:  // breakpoints, source of truth, lines are 1-based
    mutable std::mutex m_BreakpointsMutex;
    Ltg::BreakpointLines m_Breakpoints;
    std::string m_ScriptFilePathName;
    // atomic so isDebugArmed() / setDebugArmed() are lock-free; shouldArmDebug() still locks (reads breakpoints too).
    std::atomic<bool> m_DebugArmed{false};
    // monotonic counter bumped (release) at every breakpoints/scriptPath mutation. UI side reads it
    // with acquire to decide if its cached derivatives (0-based set, etc.) need a refresh — avoids
    // a per-frame mutex+set copy.
    std::atomic<int64_t> m_BreakpointsRevision{0};

public:
    // IScriptDebugHost — called by the plugin from the worker thread
    Ltg::DebugAction onPause(const Ltg::DebugState& aState) final;
    Ltg::DebugAction waitAction() final;
    void publishExpansion(int32_t aRef, const std::vector<Ltg::DebugVar>& aChildren) final;
    void publishEvalResult(int32_t aEvalId, const Ltg::EvalResult& aResult) final;
    bool isBreakpoint(int32_t aLine) final;
    bool shouldPauseOnError() const final;

    // session binding — called by ScriptingEngine around the parse run
    void bindPlugin(Ltg::IScriptDebugger* apPluginDebugger);
    void unbindPlugin();
    bool shouldArmDebug() const;
    // const& return — UI-thread writers only, the worker reads via isBreakpoint() under mutex. Caller
    // must not call setBreakpoint/toggleBreakpoint/clearBreakpoints while iterating the returned ref.
    const Ltg::BreakpointLines& getBreakpoints() const;
    int64_t getBreakpointsRevision() const;  // atomic acquire load; no mutex

    // breakpoints — UI thread, lines are 1-based
    void setBreakpoint(const std::string& aScriptFilePathName, int32_t aLine, bool aAdd);
    void toggleBreakpoint(const std::string& aScriptFilePathName, int32_t aLine);
    void clearBreakpoints();

    // full reset — disarm, clear breakpoints + script path + paused snapshot. called by
    // ProjectFile::ClearDatas on New/Open/Close (worker must already be joined by then).
    void Clear();
    const std::string& getScriptFilePathName() const;  // const& — same UI-thread invariant as getBreakpoints()

    // debug arming — UI thread
    void setDebugArmed(bool aArmed);
    bool isDebugArmed() const;

    // execution control — UI thread
    Mode getMode() const;
    Ltg::DebugState getState() const;
    int64_t getStateRevision() const;  // atomic acquire load; no mutex
    void doContinue();
    void stepInto();
    void stepOver();
    void stepOut();
    void pause();
    void stop();

    // lazy node expansion — UI thread
    void requestExpand(int32_t aRef);
    bool getChildren(int32_t aRef, std::vector<Ltg::DebugVar>& aoChildren) const;

    // watch evaluation — UI thread. requestEval posts the request once per evalId until a result
    // is cached; getEvalResult returns true when the worker has published one.
    void requestEval(int32_t aEvalId, const std::string& aExpression);
    bool getEvalResult(int32_t aEvalId, Ltg::EvalResult& aoResult) const;

private:
    void m_setCommand(Ltg::DebugCommand aCommand);
    Ltg::DebugAction m_waitNextAction();
};

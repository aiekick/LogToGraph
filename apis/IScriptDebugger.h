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
#pragma warning(disable : 4251)

#include <string>
#include <vector>
#include <cstdint>
#include <unordered_set>

namespace Ltg {

// Common debugger contract shared by the host and every scripting plugin.
//
// Line-number convention: every line number here (DebugFrame::line,
// DebugState::line, and the entries of BreakpointLines) is 1-based, i.e. the
// number Lua reports through lua_getinfo("l") and the number the user sees in
// the editor gutter. The 0-based <-> 1-based conversion to the TextEditor
// widget happens in the CodeEditor, not here.

// The command the host hands back to the paused plugin to drive execution.
enum class DebugCommand {
    Continue,  // resume until the next breakpoint
    StepInto,  // stop on the next executed line, entering called functions
    StepOver,  // stop on the next line at the same call depth
    StepOut,   // stop after the current function returns
    Stop       // abort the current script execution
};

// One entry of the paused call stack.
struct DebugFrame {
    std::string function;  // function name, empty if anonymous
    std::string source;    // source chunk name as reported by the runtime
    int32_t line = 0;      // current line in this frame (1-based)
};

// One inspected variable (local or upvalue). The value is stringified by the
// plugin because the host cannot read a runtime-native value (e.g. a Lua TValue).
struct DebugVar {
    std::string name;
    std::string value;     // already stringified by the plugin
    std::string typeName;  // runtime type name, for display
};

// Immutable snapshot of the paused state, built by the plugin in the worker
// thread and copied to the host. It holds no pointer into the runtime, so it is
// safe to read from the UI thread.
struct DebugState {
    std::string sourceFile;             // script file being executed
    int32_t line = 0;                   // current execution line (1-based)
    int32_t logRowIndex = 0;            // index of the log row currently parsed
    std::vector<DebugFrame> callStack;  // innermost frame first
    std::vector<DebugVar> locals;       // locals of the current frame
    std::vector<DebugVar> upvalues;     // upvalues of the current frame
};

// Set of breakpoint lines (1-based) for the active script.
using BreakpointLines = std::unordered_set<int32_t>;

// Implemented by the host (ScriptDebugger), called by the plugin from the
// worker thread. onPause blocks the worker until the user issues a command from
// the UI thread, then returns that command.
struct IScriptDebugHost {
    virtual ~IScriptDebugHost() = default;
    virtual DebugCommand onPause(const DebugState& aState) = 0;
};

// Implemented by the plugin (extended by ScriptingModule), called by the host
// from the UI thread to arm/drive the debug session. The control commands
// (continue/step/stop) are NOT pushed through here: they are the return value of
// IScriptDebugHost::onPause. Only the asynchronous requests live here.
struct IScriptDebugger {
    virtual ~IScriptDebugger() = default;
    // Default no-ops so a scripting plugin that does not support debugging compiles
    // unchanged (e.g. PythonScripting); the Lua plugin overrides them all.
    // give the plugin the host rendezvous interface, enabling the native hook
    virtual void enableDebug(IScriptDebugHost* /*apHost*/) {}
    // remove the native hook and resume full-speed execution
    virtual void disableDebug() {}
    // replace the breakpoint set the hook tests against
    virtual void setBreakpoints(const BreakpointLines& /*aLines*/) {}
    // ask to pause at the next executed line (asynchronous)
    virtual void requestPause() {}
    // ask to abort the current execution (asynchronous)
    virtual void requestStop() {}
};

}  // namespace Ltg

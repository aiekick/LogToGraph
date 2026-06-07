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

// The control command the host hands back to the paused plugin.
enum class DebugCommand {
    Continue,  // resume until the next breakpoint
    StepInto,  // stop on the next executed line, entering called functions
    StepOver,  // stop on the next line at the same call depth
    StepOut,   // stop after the current function returns
    Stop       // abort the current script execution
};

// One inspected value: a stack local/upvalue, a global, or a table entry.
// Values are stringified by the plugin (the host cannot read a Lua TValue). A
// table exposes a non-negative `ref` (a Lua registry reference) so the UI can
// expand it lazily; scalars/functions/userdata stay leaves with ref == -1.
struct DebugVar {
    std::string name;
    std::string keyType;   // type of the key ("none" for stack locals/upvalues)
    std::string typeName;  // type of the value
    std::string value;     // stringified value (address for table/function)
    int32_t ref = -1;      // Lua registry ref when expandable, else -1
};

// One call-stack frame; carries its own locals and upvalues (roots only, lazy).
struct DebugFrame {
    std::string function;
    std::string source;
    int32_t line = 0;
    std::vector<DebugVar> locals;
    std::vector<DebugVar> upvalues;
};

// Immutable snapshot of the paused state. Only roots are captured up front;
// tables are expanded on demand through their `ref`. It holds no pointer into
// the runtime, so it is safe to read from the UI thread.
struct DebugState {
    std::string sourceFile;
    int32_t line = 0;
    int32_t logRowIndex = 0;
    std::vector<DebugFrame> callStack;  // innermost frame first; each frame holds its vars
    std::vector<DebugVar> globals;      // top-level _G entries (no filter)
    // true when the pause was triggered by the auto-bp-on-error path (plugin caught a
    // sol2/Lua exception, queried IScriptDebugHost::shouldPauseOnError, and called onPause
    // synchronously with the throwing frame still on the Lua stack). Lets the host distinguish
    // an error pause from a breakpoint hit — used to auto-set a breakpoint at the error line
    // so subsequent runs can re-investigate.
    bool errorPause = false;
    std::string errorMessage;  // full Lua error text (used by the host as the auto-set bp tooltip / log)
};

// Set of breakpoint lines (1-based) for the active script.
using BreakpointLines = std::unordered_set<int32_t>;

// What the paused plugin should do next: resume with a command, read the children of an
// expandable node, or evaluate a watch expression in the current paused frame.
struct DebugAction {
    enum class Kind { Command, Expand, Eval };
    Kind kind = Kind::Command;
    DebugCommand command = DebugCommand::Continue;  // when kind == Command
    int32_t expandRef = -1;                         // when kind == Expand (a registry ref)
    int32_t evalId = -1;                            // when kind == Eval — caller-assigned id used to route the result back
    std::string evalExpression;                     // when kind == Eval — the Lua expression to evaluate in the paused frame
};

// Result of an evaluation in the paused frame, published asynchronously through publishEvalResult.
// `error` is non-empty on parse/runtime failure; in that case `value` / `typeName` are unspecified.
struct EvalResult {
    std::string value;
    std::string typeName;
    std::string error;
};

// Implemented by the host (ScriptDebugger), called by the plugin from the worker
// thread. The plugin loops:
//   action = onPause(state)
//   while action is Expand: publishExpansion(ref, children); action = waitAction()
//   while action is Eval: publishEvalResult(id, value, typeName, error); action = waitAction()
//   apply action.command
struct IScriptDebugHost {
    virtual ~IScriptDebugHost() = default;
    // publish the paused snapshot (roots) and block until the first action
    virtual DebugAction onPause(const DebugState& aState) = 0;
    // block until the next action (after an expansion / evaluation has been handled)
    virtual DebugAction waitAction() = 0;
    // publish the lazily-read children of an expandable node (non-blocking)
    virtual void publishExpansion(int32_t aRef, const std::vector<DebugVar>& aChildren) = 0;
    // publish the result of an evaluation request — routed back to the caller by `aEvalId`
    virtual void publishEvalResult(int32_t aEvalId, const EvalResult& aResult) = 0;
    // queried by the plugin hook on each line: is there a breakpoint on this line ?
    // live + thread-safe, so add/remove during a session takes effect immediately
    virtual bool isBreakpoint(int32_t aLine) = 0;
    // queried by the plugin catch block when a runtime error fires: should the worker pause
    // synchronously (state.errorPause = true) instead of just logging the error and moving on?
    // live so the toggle takes effect mid-run.
    virtual bool shouldPauseOnError() const = 0;
};

// Implemented by the plugin (extended by ScriptingModule), called by the host from
// the UI thread. Default no-ops so a scripting plugin that does not support
// debugging compiles unchanged (e.g. PythonScripting); the Lua plugin overrides them.
struct IScriptDebugger {
    virtual ~IScriptDebugger() = default;
    virtual void enableDebug(IScriptDebugHost* /*apHost*/) {}
    virtual void disableDebug() {}
    virtual void setBreakpoints(const BreakpointLines& /*aLines*/) {}
    virtual void requestPause() {}
    virtual void requestStop() {}
};

}  // namespace Ltg

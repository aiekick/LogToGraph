#pragma once

#include <imguipack.h>
#include <apis/IScriptDebugger.h>

#include <vector>
#include <cstdint>

// Shared rendering of the debugger variable tree (5 columns + lazy expansion),
// reused by the Stack Tree, Scope and (later) Watcher panes — single source of truth.
namespace LtgDebugUI {

// set up the 5 columns + header row of a debug-var table (call right after BeginTable)
void setupTreeColumns();

// draw the 5 columns of a tree row and return whether it is open.
// the caller owns the matching ImGui::PopID() (after children + TreePop).
bool beginTreeRow(const char* aName,
                  int32_t aDepth,
                  int32_t aIndex,
                  const char* aKeyType,
                  const char* aValueType,
                  const char* aValue,
                  int32_t& aoUid,
                  bool aDefaultOpen,
                  bool aLeaf);

// recursively render a DebugVar; tables (ref >= 0) are expanded on demand via the host
void drawLazyVar(const Ltg::DebugVar& aVar, int32_t aDepth, int32_t aIndex, int32_t& aoUid);

// render a named group (e.g. "Locals") holding a list of root vars
void drawVarGroup(const char* aName, int32_t aDepth, int32_t aIndex, const std::vector<Ltg::DebugVar>& aVars, int32_t& aoUid);

}  // namespace LtgDebugUI

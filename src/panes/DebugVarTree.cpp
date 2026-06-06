// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include "DebugVarTree.h"
#include <imgui_internal.h>
#include <models/debug/ScriptDebugger.h>

#include <string>

namespace LtgDebugUI {

void setupTreeColumns() {
    ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed);
    ImGui::TableSetupColumn("Level", ImGuiTableColumnFlags_WidthFixed);
    ImGui::TableSetupColumn("Key Type", ImGuiTableColumnFlags_WidthFixed);
    ImGui::TableSetupColumn("Value Type", ImGuiTableColumnFlags_WidthFixed);
    ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
    ImGui::TableSetupScrollFreeze(0, 1);
    ImGui::TableHeadersRow();
}

bool beginTreeRow(const char* aName,
                  int32_t aDepth,
                  int32_t aIndex,
                  const char* aKeyType,
                  const char* aValueType,
                  const char* aValue,
                  int32_t& aoUid,
                  bool aDefaultOpen,
                  bool aLeaf) {
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::PushID(aoUid++);
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth;
    if (aLeaf) {
        flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_Bullet | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    }
    if (aDefaultOpen) {
        flags |= ImGuiTreeNodeFlags_DefaultOpen;
    }
    const bool open = ImGui::TreeNodeEx(aName, flags);
    ImGui::TableNextColumn();
    ImGui::Text("%d:%d", aDepth, aIndex);
    ImGui::TableNextColumn();
    if (aKeyType != nullptr) {
        ImGui::TextUnformatted(aKeyType);
    }
    ImGui::TableNextColumn();
    if (aValueType != nullptr) {
        ImGui::TextUnformatted(aValueType);
    }
    ImGui::TableNextColumn();
    if (aValue != nullptr) {
        ImGui::TextUnformatted(aValue);
    }
    return open;
}

void drawLazyVar(const Ltg::DebugVar& aVar, int32_t aDepth, int32_t aIndex, int32_t& aoUid) {
    const bool expandable = (aVar.ref >= 0);
    const bool open =
        beginTreeRow(aVar.name.c_str(), aDepth, aIndex, aVar.keyType.c_str(), aVar.typeName.c_str(), aVar.value.c_str(), aoUid, false, !expandable);
    if (open && expandable) {
        ScriptDebugger::ref()->requestExpand(aVar.ref);
        std::vector<Ltg::DebugVar> children;
        if (ScriptDebugger::ref()->getChildren(aVar.ref, children)) {
            int32_t childIndex = 1;
            for (const auto& child : children) {
                drawLazyVar(child, aDepth + 1, childIndex++, aoUid);
            }
        } else {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextDisabled("loading...");
        }
        ImGui::TreePop();
    }
    ImGui::PopID();
}

void drawVarGroup(const char* aName, int32_t aDepth, int32_t aIndex, const std::vector<Ltg::DebugVar>& aVars, int32_t& aoUid) {
    const std::string itemsText = std::to_string(aVars.size()) + " Items";
    const bool open = beginTreeRow(aName, aDepth, aIndex, nullptr, nullptr, itemsText.c_str(), aoUid, true, false);
    if (open) {
        int32_t childIndex = 1;
        for (const auto& var : aVars) {
            drawLazyVar(var, aDepth + 1, childIndex++, aoUid);
        }
        ImGui::TreePop();
    }
    ImGui::PopID();
}

}  // namespace LtgDebugUI

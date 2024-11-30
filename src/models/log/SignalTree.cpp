#include <models/log/SignalTree.h>
#include <models/log/LogEngine.h>
#include <models/log/SignalSerie.h>
#include <ezlibs/ezStr.hpp>
#include <project/ProjectFile.h>
#include <panes/LogPane.h>

void SignalTree::clear() {
    m_RootItem.clear();
    m_SignalSeriesOld.clear();
}

void SignalTree::prepare(const std::string& vSearchString) {
    searchPattern = vSearchString;
    m_RootItem.clear();
    m_SignalSeriesOld.clear();

    for (const auto& signal_cnt : LogEngine::Instance()->GetSignalSeries()) {
        prepareRecurs(vSearchString, signal_cnt.first, signal_cnt.second, m_RootItem);
    }
}

void SignalTree::prepareRecurs(const std::string& vSearchString, const SignalCategory& vCategory, const SignalContainer& vSignals, SignalItem& vSignalItemRef) {
    std::string left_category = vCategory, right_category;
    size_t p = left_category.find('/');
    if (p != std::string::npos) {
        right_category = left_category.substr(p + 1);  // right part
        left_category = left_category.substr(0, p);    // legt part
    }

    if (vSignalItemRef.childs.find(left_category) == vSignalItemRef.childs.end()) {  // not found, we need to insert a item
        vSignalItemRef.childs[left_category];
    }

    auto& item = vSignalItemRef.childs.at(left_category);
    if (!right_category.empty()) {  // no leaf, recurs
        prepareRecurs(vSearchString, right_category, vSignals, item);
        item.count = static_cast<uint32_t>(item.childs.size());

    } else {  // leaf : add
        item.count = static_cast<uint32_t>(vSignals.size());
        for (const auto& sig : vSignals) {
            if (sig.second != nullptr) {
                if (vSearchString.empty() || sig.second->low_case_name_for_search.find(vSearchString) != std::string::npos) {
                    item.signals[sig.first] = sig.second;
                }
            }
        }
    }
    item.label = ez::str::toStr("%s (%u)", left_category.c_str(), item.count);

    // we will remove items with empty childs and empty signals
    // can be the case if search pattern filtering blocked some insertions
    if (item.isEmpty()) {
        vSignalItemRef.childs.erase(left_category);
    }
}

void SignalTree::displayTree(bool vCollapseAll, bool vExpandAll) {
    displayItemRecurs(m_RootItem, vCollapseAll, vExpandAll);
}

void SignalTree::displayItemRecurs(SignalItem& vSignalItemRef, bool vCollapseAll, bool vExpandAll) {
    if (vSignalItemRef.isLeaf()) {
        for (auto& signal : vSignalItemRef.signals) {
            if (!signal.second.expired()) {
                auto ptr = signal.second.lock();
                if (ptr) {
                    if (ImGui::Selectable(ptr->label.c_str(), ptr->show)) {
                        ptr->show = !ptr->show;
                        LogEngine::Instance()->ShowHideSignal(ptr->category, ptr->name, ptr->show);
                        if (ProjectFile::Instance()->m_CollapseLogSelection) {
                            LogPane::Instance()->PrepareLog();
                        }
                        ProjectFile::Instance()->SetProjectChange();
                    }
                }
            }
        }
    } else {  // display categories
        for (auto& child : vSignalItemRef.childs) {
            if (vCollapseAll) {
                // can close only the first item for now
                // or we need to reach the leaf
                // and close from leaf so to do on many frames
                // can be anoying for the user
                // todo : by the way
                ImGui::SetNextItemOpen(false);
            }

            if (vExpandAll) {
                // will open all tree during recursion
                ImGui::SetNextItemOpen(true);
            }

            if (ImGui::TreeNode(&child.second, "%s", child.second.label.c_str())) {
                ImGui::Indent();
                displayItemRecurs(child.second, vCollapseAll, vExpandAll);
                ImGui::Unindent();
                ImGui::TreePop();
            }
        }
    }
}

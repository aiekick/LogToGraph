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
        /*const auto& arr = ez::str::splitStringToVector(item_cat.first, "/");
        for (auto& item_name : item_cat.second) {
            if (item_name.second) {
                if (item_name.second->low_case_name_for_search.find(searchPattern) == std::string::npos) {
                    continue;
                }
                m_SignalSeriesOld[item_name.first] = item_name.second;
                m_SignalSeries[item_name.first];
            }
        }*/
    }
}

void SignalTree::prepareRecurs(const std::string& vSearchString, const SignalCategory& vCategory, const SignalContainer& vSignals, SignalItem& vSignalItemRef) {
    // split category
    std::string left_category = vCategory, right_category;
    size_t p = left_category.find('/');
    if (p != std::string::npos) {
        right_category = left_category.substr(p + 1);  // right part
        left_category = left_category.substr(0, p);    // legt part
    }

    if (vSignalItemRef.childs.find(left_category) == vSignalItemRef.childs.end()) {  // we need to insert a item
        vSignalItemRef.childs[left_category];
    }

    auto& item = vSignalItemRef.childs.at(left_category);
    if (!right_category.empty()) {  // no leaf, recurs
        prepareRecurs(vSearchString, right_category, vSignals, item);
        item.count = static_cast<uint32_t>(item.childs.size());
    } else {  // leaf : add
        item.count = static_cast<uint32_t>(vSignals.size());
        for (const auto& sig : vSignals) {
            item.signals[sig.first] = sig.second;
        }
    }
    item.label = ez::str::toStr("%s (%u)", left_category.c_str(), item.count);
}

void SignalTree::displayTree(bool vCollapseAll, bool vExpandAll) {
    displayItemRecurs(m_RootItem);
}

void SignalTree::displayItemRecurs(SignalItem& vSignalItemRef) {
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
            if (ImGui::TreeNode(child.second.label.c_str())) {
                ImGui::Indent();
                displayItemRecurs(child.second);
                ImGui::Unindent();
                ImGui::TreePop();
            }
        }
    }
}

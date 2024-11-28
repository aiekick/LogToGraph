#include <models/log/SignalTree.h>
#include <models/log/LogEngine.h>
#include <models/log/SignalSerie.h>
#include <ezlibs/ezStr.hpp>
#include <project/ProjectFile.h>
#include <panes/LogPane.h>

void SignalTree::clear() {
    m_SignalSeries.clear();
    m_SignalSeriesOld.clear();
}

void SignalTree::prepare(const std::string& vSearchString) {
    searchPattern = vSearchString;
    m_SignalSeries.clear();
    m_SignalSeriesOld.clear();

    for (auto& item_cat : LogEngine::Instance()->GetSignalSeries()) {
        for (auto& item_name : item_cat.second) {
            if (item_name.second) {
                if (item_name.second->low_case_name_for_search.find(searchPattern) == std::string::npos) {
                    continue;
                }
                m_SignalSeriesOld[item_name.first] = item_name.second;
                // const auto& arr = ez::str::splitStringToVector(item_name.first, "/");
                // m_SignalSeries[item_name.first] = item_name.second;
            }
        }
    }
}

void SignalTree::prepareRecurs(const std::string& vSearchString, const std::string& vName, const SignalSeriePtr& vSignalSeriePtr) {
    if (!vName.empty() && vSignalSeriePtr != nullptr) {
        /*if (vSignalSeriePtr->low_case_name_for_search.find(vSearchString) == std::string::npos) {
            return
        }*/
        std::string base_name = vName;
        size_t p = base_name.find('/');
        if (p != std::string::npos) {
            std::string name = base_name.substr(0, p);
            std::string rest = base_name.substr(p + 1);
        } else {
        
        }
        //const auto& arr = ez::str::splitStringToVector(vName, "/");
        //m_SignalSeries[item_name.first] = item_name.second;
    }
}

void SignalTree::displayTree(bool vCollapseAll, bool vExpandAll) {
    if (!searchPattern.empty()) {
        // if first frame is not built
        if (m_SignalSeries.empty()) {
            prepare(searchPattern);
        }

        ImGui::Indent();

        // affichage ordonne sans les categorie
        for (auto& item_name : m_SignalSeriesOld) {
            displayItemRecurs(item_name.second);
        }

        ImGui::Unindent();
    } else {
        // affichage arborescent ordonne par categorie
        for (auto& item_cat : LogEngine::Instance()->GetSignalSeries()) {
            if (vCollapseAll) {
                ImGui::SetNextItemOpen(false);
            }

            if (vExpandAll) {
                ImGui::SetNextItemOpen(true);
            }

            auto cat_str = ez::str::toStr("%s (%u)", item_cat.first.c_str(), (uint32_t)item_cat.second.size());
            if (ImGui::TreeNode(cat_str.c_str())) {
                ImGui::Indent();

                for (auto& item_name : item_cat.second) {
                    displayItemRecurs(item_name.second);
                }

                ImGui::Unindent();

                ImGui::TreePop();
            }
        }
    }
}

void SignalTree::displayItemRecurs(const SignalSerieWeak& vDatasSerie) {
    if (!vDatasSerie.expired()) {
        auto ptr = vDatasSerie.lock();
        if (ptr) {
            auto name_str = ez::str::toStr("%s (%u)", ptr->name.c_str(), (uint32_t)ptr->count_base_records);
            if (ImGui::Selectable(name_str.c_str(), ptr->show)) {
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

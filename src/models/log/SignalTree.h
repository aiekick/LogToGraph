#pragma once

#include <headers/DatasDef.h>
#include <string>
#include <map>

struct SignalItem;
typedef std::map<SignalName, SignalItem> SignalItemContainer;
struct SignalItem {
    uint32_t count = 0U;
    std::string label; // label displayed in imgui tree
    SignalContainerWeak signals;
    SignalItemContainer childs;
    bool isLeaf() const { return childs.empty(); }
    bool isEmpty() const { return childs.empty() && signals.empty(); }
    void clear() {
        signals.clear();
        childs.clear();
    }
};

class SignalTree {
private:
    std::string searchPattern;
    SignalItem m_RootItem;
    std::map<SignalName, SignalSerieWeak> m_SignalSeriesOld;

public:
    void clear();
    void prepare(const std::string& vSearchString);
    void displayTree(bool vCollapseAll, bool vExpandAll);

private:
    void prepareRecurs(const std::string& vSearchString, const SignalCategory& vCategory, const SignalContainer& vSignals, SignalItem& vSignalItemRef);
    void displayItemRecurs(SignalItem& vSignalItemRef, bool vCollapseAll, bool vExpandAll);
};
#pragma once

#include <headers/DatasDef.h>
#include <string>
#include <map>


class SignalItem {
public:
    uint32_t count = 0U;
    std::string label;  // label displayed in imgui tree
    SignalContainerWeak signals;
    SignalItemContainer childs;

public:
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

public:
    void clear();
    void prepare(const std::string& vSearchString);
    const SignalItem& getRootItem() const;
    void displayTree(bool vCollapseAll, bool vExpandAll);

private:
    void prepareRecurs(const std::string& vSearchString, const SignalCategory& vCategory, const SignalContainer& vSignals, SignalItem& vSignalItemRef);
    void displayItemRecurs(SignalItem& vSignalItemRef, bool vCollapseAll, bool vExpandAll);
};
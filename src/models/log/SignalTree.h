#pragma once

#include <headers/DatasDef.h>
#include <string>
#include <map>

struct SignalItem {
    SignalName name;
    SignalSerieWeak signal;
    std::map<SignalName, SignalItem> items;
};

class SignalTree {
private:
    std::string searchPattern;
    std::map<SignalName, SignalItem> m_SignalSeries;
    std::map<SignalName, SignalSerieWeak> m_SignalSeriesOld;

public:
    void clear();
    void prepare(const std::string& vSearchString);
    void displayTree(bool vCollapseAll, bool vExpandAll);

private:
    void prepareRecurs(const std::string& vSearchString, const std::string& vName, const SignalSeriePtr& vSignalSeriePtr);
    void displayItemRecurs(const SignalSerieWeak& vDatasSerie);
};
#pragma once

#include <res/fontIcons.h>

#define APP_TITLE "LogToGraph"
#define APP_PROJECT_FILE_EXT "ltg"

#include <map>
#include <list>
#include <vector>
#include <string>
#include <memory>
#include <ImGuiPack.h>
#include <ezlibs/ezTools.hpp>
#include <ezlibs/ezXmlConfig.hpp>

#define declareMember(type, var, def)   \
private:                                \
    type m_##var = def;                 \
                                        \
public:                                 \
    void set##var(const type& v##var) { \
        m_##var = v##var;               \
    }                                   \
    type& get##var##Ref() {             \
        return m_##var;                 \
    }                                   \
    const type& get##var() const {      \
        return m_##var;                 \
    }

typedef const char* ImGuiLabel;

typedef double SignalValue;
typedef std::string SignalString;
typedef std::string SignalStatus;
typedef std::string SignalDesc;

typedef ez::dvec2 SignalValueRange;
typedef const ez::dvec2& SignalValueRangeConstRef;

typedef uint32_t SignalColor;

typedef double SignalEpochTime;

typedef ImVec4 SignalTagColor;
typedef std::string SignalTagName;
typedef std::string SignalTagHelp;

typedef std::string SignalName;
typedef int32_t SignalNameID;

typedef std::string DBFile;

typedef std::string SignalCategory;
typedef int32_t SignalCategoryID;

typedef int32_t DBRowID;

typedef std::string SignalDateTime;

typedef double EpochOffset;

typedef std::string SourceFilePathName;
typedef std::string SourceFileName;
typedef int32_t SourceFileID;

class SourceFile;
typedef std::shared_ptr<SourceFile> SourceFilePtr;
typedef std::weak_ptr<SourceFile> SourceFileWeak;

typedef std::map<SourceFileName, SourceFilePtr> SourceFilesContainer;
typedef std::map<SourceFileName, SourceFilePtr>& SourceFilesContainerRef;

class SignalTick;
typedef std::shared_ptr<SignalTick> SignalTickPtr;
typedef std::weak_ptr<SignalTick> SignalTickWeak;

typedef std::vector<SignalTickPtr> SignalTicksContainer;
typedef std::vector<SignalTickPtr>& SignalTicksContainerRef;

typedef std::vector<SignalTickWeak> SignalTicksWeakContainer;
typedef std::vector<SignalTickWeak>& SignalTicksWeakContainerRef;

typedef std::vector<std::pair<SignalTickWeak, SignalTickWeak>> SignalDiffWeakContainer;
typedef std::vector<std::pair<SignalTickWeak, SignalTickWeak>>& SignalDiffWeakContainerRef;

class SignalTag;
typedef std::shared_ptr<SignalTag> SignalTagPtr;
typedef std::weak_ptr<SignalTag> SignalTagWeak;

typedef std::vector<SignalTagPtr> SignalTagsContainer;
typedef std::vector<SignalTagPtr>& SignalTagsContainerRef;

typedef std::vector<SignalTagWeak> SignalTagsWeakContainer;
typedef std::vector<SignalTagWeak>& SignalTagsWeakContainerRef;

class SignalSerie;
typedef std::shared_ptr<SignalSerie> SignalSeriePtr;
typedef std::weak_ptr<SignalSerie> SignalSerieWeak;

typedef std::map<SignalName, SignalSeriePtr> SignalContainer;
typedef std::map<SignalCategory, SignalContainer> SignalSeriesContainer;
typedef std::map<SignalCategory, SignalContainer>& SignalSeriesContainerRef;

typedef std::map<SignalName, SignalSerieWeak> SignalContainerWeak;
typedef std::map<SignalCategory, SignalContainerWeak> SignalSeriesWeakContainer;
typedef std::map<SignalCategory, SignalContainerWeak>& SignalSeriesWeakContainerRef;

class GraphGroup;
typedef std::shared_ptr<GraphGroup> GraphGroupPtr;
typedef std::weak_ptr<GraphGroup> GraphGroupWeak;

typedef std::list<GraphGroupPtr> GraphGroups;
typedef std::list<GraphGroupPtr>& GraphGroupsRef;

class GraphAnnotation;
typedef std::shared_ptr<GraphAnnotation> GraphAnnotationPtr;
typedef std::weak_ptr<GraphAnnotation> GraphAnnotationWeak;

typedef std::vector<std::pair<SourceFileName, SourceFilePathName>> SourceFileContainer;

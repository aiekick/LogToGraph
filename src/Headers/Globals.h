#pragma once

#include <Contrib/FontIcons/CustomFont2.h>

#define APP_TITLE "LogToGraph"
#define APP_PROJECT_FILE_EXT "ltg"

#include <map>
#include <list>
#include <vector>
#include <string>
#include <memory>
#include <ctools/cTools.h>

typedef const char* UInt8ConstPtr;

typedef double SignalValue;

typedef ct::dvec2 SignalValueRange;
typedef const ct::dvec2& SignalValueRangeConstRef;

typedef uint32_t SignalColor;

typedef double SignalEpochTime;

typedef std::string SignalName;
typedef int32_t SignalNameID;

typedef std::string DBFile;

typedef std::string SourceFile;
typedef int32_t SourceFileID;

typedef std::string SignalCategory;
typedef int32_t SignalCategoryID;

typedef int32_t DBRowID;

typedef std::string SignalDateTime;

class SignalTick;
typedef std::shared_ptr<SignalTick> SignalTickPtr;
typedef std::weak_ptr<SignalTick> SignalTickWeak;

class SignalSerie;
typedef std::shared_ptr<SignalSerie> SignalSeriePtr;
typedef std::weak_ptr<SignalSerie> SignalSerieWeak;

typedef std::vector<SignalTickPtr> SignalTicksContainer;
typedef std::vector<SignalTickPtr>& SignalTicksContainerRef;

typedef std::vector<SignalTickWeak> SignalTicksWeakContainer;
typedef std::vector<SignalTickWeak>& SignalTicksWeakContainerRef;

typedef std::vector<std::pair<SignalTickWeak, SignalTickWeak>> SignalDiffWeakContainer;
typedef std::vector<std::pair<SignalTickWeak, SignalTickWeak>>& SignalDiffWeakContainerRef;

typedef std::map<SignalCategory, std::map<SignalName, SignalSeriePtr>> SignalSeriesContainer;
typedef std::map<SignalCategory, std::map<SignalName, SignalSeriePtr>>& SignalSeriesContainerRef;

typedef std::map<SignalCategory, std::map<SignalName, SignalSerieWeak>> SignalSeriesWeakContainer;
typedef std::map<SignalCategory, std::map<SignalName, SignalSerieWeak>>& SignalSeriesWeakContainerRef;

class GraphGroup;
typedef std::shared_ptr<GraphGroup> GraphGroupPtr;
typedef std::weak_ptr<GraphGroup> GraphGroupWeak;
typedef std::list<GraphGroupPtr> GraphGroups;
typedef std::list<GraphGroupPtr>& GraphGroupsRef;

class GraphAnnotation;
typedef std::shared_ptr<GraphAnnotation> GraphAnnotationPtr;
typedef std::weak_ptr<GraphAnnotation> GraphAnnotationWeak;

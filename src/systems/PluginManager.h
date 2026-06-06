#pragma once

#include <set>
#include <map>
#include <string>
#include <memory>
#include <filesystem>

#include <apis/LtgPluginApi.h>
#include <ezlibs/ezClass.hpp>
#include <ezlibs/ezPlugin.hpp>
#include <ezlibs/ezSingleton.hpp>

#if defined(__WIN32__) || defined(WIN32) || defined(_WIN32) || defined(__WIN64__) || defined(WIN64) || defined(_WIN64) || defined(_MSC_VER)
#if defined(Core_EXPORTS)
#define LOG_TO_GRAPH_CORE_API __declspec(dllexport)
#elif defined(BUILD_LOG_TO_GRAPH_CORE_SHARED_LIBS)
#define LOG_TO_GRAPH_CORE_API __declspec(dllimport)
#else
#define LOG_TO_GRAPH_CORE_API
#endif
#else
#define LOG_TO_GRAPH_CORE_API
#endif

class PluginInstance;
typedef std::weak_ptr<PluginInstance> PluginInstanceWeak;
typedef std::shared_ptr<PluginInstance> PluginInstancePtr;

enum class PluginReturnMsg { LOADING_SUCCEED = 1, LOADING_FAILED = 0, NOT_A_PLUGIN = -1 };

struct PluginInterface;
typedef std::shared_ptr<Ltg::PluginInterface> PluginInterfacePtr;
typedef std::weak_ptr<Ltg::PluginInterface> PluginInterfaceWeak;

class LOG_TO_GRAPH_CORE_API PluginInstance {
private:
    ez::plugin::Loader<Ltg::PluginInterface> m_Loader;
    PluginInterfacePtr m_PluginInstance = nullptr;
    std::string m_Name;

public:
    PluginInstance();
    virtual ~PluginInstance();

    PluginReturnMsg init(const std::string& vName, const std::string& vFilePathName);
    void unit();

    PluginInterfaceWeak get() const;
};

class LOG_TO_GRAPH_CORE_API PluginManager : public Ltg::PluginBridge {
    DISABLE_CONSTRUCTORS(PluginManager)
    DISABLE_DESTRUCTORS(PluginManager)
    IMPLEMENT_SINGLETON(PluginManager)

private:
    std::map<std::string, PluginInstancePtr> m_Plugins;

public:
    void loadPlugins(const std::string& vAppPath, const std::set<Ltg::PluginModuleType> vTypesToLoad = {});
    void unloadPlugins();
    std::vector<Ltg::PluginModuleInfos> getPluginModulesInfos() const;
    Ltg::PluginModulePtr createPluginModule(const std::string& vPluginNodeName);

private:
    void m_loadPlugin(const std::filesystem::directory_entry& vEntry, const std::set<Ltg::PluginModuleType> vTypesToLoad);
    void m_displayLoadedPlugins();
};
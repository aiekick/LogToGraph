#pragma once

#include <set>
#include <map>
#include <string>
#include <memory>
#include <filesystem>

#ifdef WIN32
#include <IDLLoader/Windows/DLLoader.h>
#else
#include <IDLLoader/Unix/DLLoader.h>
#endif

#include <apis/LtgPluginApi.h>

#if defined(__WIN32__) || defined(WIN32) || defined(_WIN32) || defined(__WIN64__) || defined(WIN64) || defined(_WIN64) || defined(_MSC_VER)
#if defined(Core_EXPORTS)
#define STROCKER_CORE_API __declspec(dllexport)
#elif defined(BUILD_STROCKER_CORE_SHARED_LIBS)
#define STROCKER_CORE_API __declspec(dllimport)
#else
#define STROCKER_CORE_API
#endif
#else
#define STROCKER_CORE_API
#endif

class PluginInstance;
typedef std::weak_ptr<PluginInstance> PluginInstanceWeak;
typedef std::shared_ptr<PluginInstance> PluginInstancePtr;

enum class PluginReturnMsg { LOADING_SUCCEED = 1, LOADING_FAILED = 0, NOT_A_PLUGIN = -1 };

struct PluginInterface;
typedef std::shared_ptr<Ltg::PluginInterface> PluginInterfacePtr;
typedef std::weak_ptr<Ltg::PluginInterface> PluginInterfaceWeak;

class STROCKER_CORE_API PluginInstance {
private:
    dlloader::DLLoader<Ltg::PluginInterface> m_Loader;
    PluginInterfacePtr m_PluginInstance = nullptr;
    std::string m_Name;

public:
    PluginInstance();
    virtual ~PluginInstance();

    PluginReturnMsg init(const std::string& vName, const std::string& vFilePathName);
    void unit();

    PluginInterfaceWeak get() const;
};

class STROCKER_CORE_API PluginManager : public Ltg::PluginBridge {
private:
    std::map<std::string, PluginInstancePtr> m_Plugins;

public:
    void loadPlugins(const std::string& vAppPath, const std::set<Ltg::PluginModuleType> vTypesToLoad = {});
    void unloadPlugins();
    std::vector<Ltg::PluginModuleInfos> getPluginModulesInfos() const;
    Ltg::PluginModulePtr createPluginModule(const std::string& vPluginNodeName);
    std::vector<Ltg::PluginPaneConfig> getPluginPanes() const;
    std::vector<Ltg::PluginSettingsConfig> getPluginSettings() const;

    Ltg::IndicatorComputingPtr getIndicatorPtr(const std::string& vIndicatorName); // called from a plugin for acces an indicator

private:
    void m_loadPlugin(const std::filesystem::directory_entry& vEntry, const std::set<Ltg::PluginModuleType> vTypesToLoad);
    void m_displayLoadedPlugins();

public:
    PluginManager() = default;                      // Prevent construction
    PluginManager(const PluginManager&) = default;  // Prevent construction by copying
    PluginManager& operator=(const PluginManager&) {
        return *this;
    };                           // Prevent assignment
    virtual ~PluginManager() = default;  // Prevent unwanted destruction

public:
    static PluginManager* Instance() {
        static auto _instance = std::make_unique<PluginManager>();
        return _instance.get();
    }
};
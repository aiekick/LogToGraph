#include <systems/PluginManager.h>

#include <ezlibs/ezFile.hpp>
#include <ezlibs/ezMath.hpp>
#include <ezlibs/ezLog.hpp>

namespace fs = std::filesystem;

//////////////////////////////////////////////////////////////////////////////
////// PluginInstance ////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

PluginInstance::PluginInstance() {}

PluginInstance::~PluginInstance() {
    unit();
}

PluginReturnMsg PluginInstance::init(const std::string& vName, const std::string& vFilePathName) {
    m_Name = vName;
    m_Loader = dlloader::DLLoader<Ltg::PluginInterface>(vFilePathName);
    m_Loader.DLOpenLib();
    m_PluginInstance = m_Loader.DLGetInstance();
    if (m_Loader.IsAPlugin()) {
        if (m_Loader.IsValid()) {
            if (m_PluginInstance) {
                if (!m_PluginInstance->init(  //
                        ez::Log::Instance()   // redirection of the logger instance
                        )) {
                    m_PluginInstance.reset();
                } else {
                    return PluginReturnMsg::LOADING_SUCCEED;
                }
            }
        }
        return PluginReturnMsg::LOADING_FAILED;
    }
    return PluginReturnMsg::NOT_A_PLUGIN;
}

void PluginInstance::unit() {
    if (m_PluginInstance != nullptr) {
        m_PluginInstance->unit();
    }
    m_PluginInstance.reset();
    m_Loader.DLCloseLib();
}

PluginInterfaceWeak PluginInstance::get() const {
    return m_PluginInstance;
}

//////////////////////////////////////////////////////////////////////////////
////// PluginLoader //////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

static inline std::string getDLLExtention() {
#ifdef WIN32
    return "dll";
#elif defined(__linux__)
    return "so";
#elif defined(__APPLE__)
    return "dylib";
#elif
    return "";
#endif
}

void PluginManager::unloadPlugins() {
    m_Plugins.clear();
}

void PluginManager::loadPlugins(const std::string& vAppPath, const std::set<Ltg::PluginModuleType> vTypesToLoad) {
    std::cout << "-----------" << std::endl;
    LogVarLightInfo("Availables Plugins :");
    auto plugin_directory = std::filesystem::path(vAppPath).append("plugins");
    if (std::filesystem::exists(plugin_directory)) {
        const auto dir_iter = std::filesystem::directory_iterator(plugin_directory);
        for (const auto& file : dir_iter) {
            m_loadPlugin(file, vTypesToLoad);
        }
        m_displayLoadedPlugins();
    } else {
        LogVarLightInfo("x Plugin directory %s not found !", plugin_directory.string().c_str());
    }
    std::cout << "-----------" << std::endl;
}

std::vector<Ltg::PluginModuleInfos> PluginManager::getPluginModulesInfos() const {
    std::vector<Ltg::PluginModuleInfos> res;
    for (auto plugin : m_Plugins) {
        if (plugin.second) {
            auto pluginInstancePtr = plugin.second->get().lock();
            if (pluginInstancePtr != nullptr) {
                auto lib_entrys = pluginInstancePtr->getModulesInfos();
                if (!lib_entrys.empty()) {
                    res.insert(res.end(), lib_entrys.begin(), lib_entrys.end());
                }
            }
        }
    }
    return res;
}

Ltg::PluginModulePtr PluginManager::createPluginModule(const std::string& vPluginNodeName) {
    if (!vPluginNodeName.empty()) {
        for (auto plugin : m_Plugins) {
            if (plugin.second) {
                auto pluginInstancePtr = plugin.second->get().lock();
                if (pluginInstancePtr != nullptr) {
                    auto ptr = pluginInstancePtr->createModule(vPluginNodeName, this);
                    if (ptr != nullptr) {
                        return ptr;
                    }
                }
            }
        }
    }
    return nullptr;
}

std::vector<Ltg::PluginPaneConfig> PluginManager::getPluginPanes() const {
    std::vector<Ltg::PluginPaneConfig> pluginsPanes;
    for (auto plugin : m_Plugins) {
        if (plugin.second) {
            auto pluginInstancePtr = plugin.second->get().lock();
            if (pluginInstancePtr) {
                auto _pluginPanes = pluginInstancePtr->getPanes();
                if (!_pluginPanes.empty()) {
                    pluginsPanes.insert(pluginsPanes.end(), _pluginPanes.begin(), _pluginPanes.end());
                }
            }
        }
    }
    return pluginsPanes;
}

std::vector<Ltg::PluginSettingsConfig> PluginManager::getPluginSettings() const {
    std::vector<Ltg::PluginSettingsConfig> pluginSettings;
    for (auto plugin : m_Plugins) {
        if (plugin.second) {
            auto pluginInstancePtr = plugin.second->get().lock();
            if (pluginInstancePtr) {
                auto _pluginSettings = pluginInstancePtr->getSettings();
                if (!_pluginSettings.empty()) {
                    pluginSettings.insert(pluginSettings.end(), _pluginSettings.begin(), _pluginSettings.end());
                }
            }
        }
    }
    return pluginSettings;
}

//////////////////////////////////////////////////////////////
//// PRIVATE /////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

void PluginManager::m_loadPlugin(const fs::directory_entry& vEntry, const std::set<Ltg::PluginModuleType> vTypesToLoad) {
    if (vEntry.is_directory()) {
        const auto dir_iter = std::filesystem::directory_iterator(vEntry);
        for (const auto& file : dir_iter) {
            m_loadPlugin(file, vTypesToLoad);
        }
    } else if (vEntry.is_regular_file()) {
        auto file_name = vEntry.path().filename().string();
        if (file_name.find(PLUGIN_NAME_PREFIX) == 0U) {
            if (file_name.find(PLUGIN_RUNTIME_CONFIG) != std::string::npos) {
                auto file_path_name = vEntry.path().string();
                if (file_path_name.find(getDLLExtention()) != std::string::npos) {
                    auto ps = ez::file::parsePathFileName(file_path_name);
                    if (ps.isOk) {
                        auto resPtr = std::make_shared<PluginInstance>();
                        auto ret = resPtr->init(ps.name, ps.GetFPNE());
                        if (ret != PluginReturnMsg::LOADING_SUCCEED) {
                            resPtr.reset();
                            if (ret == PluginReturnMsg::LOADING_FAILED) {
                                LogVarDebugError("x Plugin %s fail to load", ps.name.c_str());
                            }
                        } else {
                            bool authorized = false;
                            {
                                auto pluginInstancePtr = resPtr->get().lock();
                                if (pluginInstancePtr) {
                                    // we will load only authorized types, if there is at least one authorized type per plugin we load the plugin
                                    // else no types are ok for a plugin we unload it
                                    if (!vTypesToLoad.empty()) {
                                        const auto& modules = pluginInstancePtr->getModulesInfos();
                                        for (const auto& m : modules) {
                                            if (vTypesToLoad.find(m.type) != vTypesToLoad.end()) {
                                                // au moin un des type est autorisé. donc on va charger le plugin
                                                authorized = true;
                                                // pas besoin de s'eterniser
                                                break;
                                            }
                                        }
                                    } else {
                                        // no particular types to load, so we load all plugins
                                        authorized = true;
                                    }
                                }
                            }

                            if (authorized) {
                                m_Plugins[ps.name] = resPtr;
                            }
                        }
                    }
                }
            }
        }
    }
}

void PluginManager::m_displayLoadedPlugins() {
    if (!m_Plugins.empty()) {
        size_t max_name_size = 0U;
        size_t max_vers_size = 0U;
        const size_t& minimal_space = 2U;
        for (auto plugin : m_Plugins) {
            if (plugin.second != nullptr) {
                auto plugin_instance_ptr = plugin.second->get().lock();
                if (plugin_instance_ptr != nullptr) {
                    max_name_size = ez::maxi(max_name_size, plugin_instance_ptr->getName().size() + minimal_space);
                    max_vers_size = ez::maxi(max_vers_size, plugin_instance_ptr->getVersion().size() + minimal_space);
                }
            }
        }
        for (auto plugin : m_Plugins) {
            if (plugin.second != nullptr) {
                auto plugin_instance_ptr = plugin.second->get().lock();
                if (plugin_instance_ptr != nullptr) {
                    const auto& name = plugin_instance_ptr->getName();
                    const auto& name_space = std::string(max_name_size - name.size(), ' ');  // 32 is a space in ASCII
                    const auto& vers = plugin_instance_ptr->getVersion();
                    const auto& vers_space = std::string(max_vers_size - vers.size(), ' ');  // 32 is a space in ASCII
                    const auto& desc = plugin_instance_ptr->getDescription();
                    LogVarLightInfo("- Plugin loaded : %s%sv%s%s(%s)",  //
                                    name.c_str(),
                                    name_space.c_str(),  //
                                    vers.c_str(),
                                    vers_space.c_str(),  //
                                    desc.c_str());
                }
            }
        }
    }
}
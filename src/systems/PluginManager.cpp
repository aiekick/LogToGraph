#include <core/managers/PluginManager.h>

#include <EzLibs/EzFile.hpp>
#include <EzLibs/EzMath.hpp>

namespace fs = std::filesystem;

//////////////////////////////////////////////////////////////////////////////
////// PluginInstance ////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

PluginInstance::PluginInstance() {
}

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
                if (!m_PluginInstance->Init()) {
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
        m_PluginInstance->Unit();
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
    printf("-----------\n");
    LogVarLightInfo("Availables Plugins :\n");
    auto plugin_directory = std::filesystem::path(vAppPath).append("plugins");
    if (std::filesystem::exists(plugin_directory)) {
        const auto dir_iter = std::filesystem::directory_iterator(plugin_directory);
        for (const auto& file : dir_iter) {
            m_loadPlugin(file, vTypesToLoad);
        }
        m_displayLoadedPlugins();
    } else {
        LogVarLightInfo("Plugin directory %s not found !", plugin_directory.string().c_str());
    }
    printf("-----------\n");
}

Ltg::IndicatorComputingPtr PluginManager::getIndicatorPtr(const std::string& vIndicatorName) {
    auto ptr = createPluginModule(vIndicatorName);
    if (ptr != nullptr) {
        auto indicatorPtr = std::dynamic_pointer_cast<Ltg::IndicatorComputing>(ptr);
        if (indicatorPtr != nullptr) {
            return indicatorPtr; // the plugin will retain the pointer, he must free it
        } else {
            LogVarError("The Module %s is not an Indicator", vIndicatorName.c_str());
        }
    }
    return nullptr;
}

std::vector<Ltg::PluginModuleInfos> PluginManager::getPluginModulesInfos() const {
    std::vector<Ltg::PluginModuleInfos> res;

    for (auto plugin : m_Plugins) {
        if (plugin.second) {
            auto pluginInstancePtr = plugin.second->get().lock();
            if (pluginInstancePtr != nullptr) {
                auto lib_entrys = pluginInstancePtr->GetModulesInfos();
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
                    auto ptr = pluginInstancePtr->CreateModule(vPluginNodeName, this);
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
                auto _pluginPanes = pluginInstancePtr->GetPanes();
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
                auto _pluginSettings = pluginInstancePtr->GetSettings();
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
                                LogVarDebugError("Plugin %s fail to load", ps.name.c_str());
                            }
                        } else {
                            bool authorized = false;
                            {
                                auto pluginInstancePtr = resPtr->get().lock();
                                if (pluginInstancePtr) {
                                    // we will load only authorized types, if there is at least one authorized type per plugin we load the plugin
                                    // else no types are ok for a plugin we unload it
                                    if (!vTypesToLoad.empty()) {
                                        const auto& modules = pluginInstancePtr->GetModulesInfos();
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
                    max_name_size = ez::maxi(max_name_size, plugin_instance_ptr->GetName().size() + minimal_space);
                    max_vers_size = ez::maxi(max_vers_size, plugin_instance_ptr->GetVersion().size() + minimal_space);
                }
            }
        }
        for (auto plugin : m_Plugins) {
            if (plugin.second != nullptr) {
                auto plugin_instance_ptr = plugin.second->get().lock();
                if (plugin_instance_ptr != nullptr) {
                    const auto& name = plugin_instance_ptr->GetName();
                    const auto& name_space = std::string(max_name_size - name.size(), ' ');  // 32 is a space in ASCII
                    const auto& vers = plugin_instance_ptr->GetVersion();
                    const auto& vers_space = std::string(max_vers_size - vers.size(), ' ');  // 32 is a space in ASCII
                    const auto& desc = plugin_instance_ptr->GetDescription();
                    LogVarLightInfo("Plugin loaded : %s%sv%s%s(%s)",  //
                        name.c_str(), name_space.c_str(),             //
                        vers.c_str(), vers_space.c_str(),             //
                        desc.c_str());
                }
            }
        }
    }
}
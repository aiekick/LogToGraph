/*
Copyright 2022-2024 Stephane Cuillerdier (aka aiekick)

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#pragma once
#pragma warning(disable : 4251)

#include <memory>
#include <cstdint>
#include <string>
#include <vector>
#include <array>
#include <map>

// The plugin API is deliberately ImGui-free: plugins are pure modules (scripting + data) and link
// no imguipack/glad/glfw. The host owns all UI. The only host objects a plugin borrows are
// forwarded by pointer/interface at instantiation (ez::Log via init(), IDatasModel via load(),
// IScriptDebugHost via enableDebug()). This is what lets LogToGraph build fully static. The host
// settings-dialog interface (ISettings) is NOT here anymore — it moved to src/systems/ISettings.h
// since plugins no longer provide settings.
#include "IScriptDebugger.h"

namespace ez {
class Log;
}

namespace Ltg {

class IProject {
public:
    virtual bool IsProjectLoaded() const = 0;
    virtual bool IsProjectNeverSaved() const = 0;
    virtual bool IsThereAnyProjectChanges() const = 0;
    virtual void SetProjectChange(bool vChange = true) = 0;
    virtual bool WasJustSaved() = 0;
};
typedef std::shared_ptr<IProject> IProjectPtr;
typedef std::weak_ptr<IProject> IProjectWeak;

struct PluginParam {
    std::string name;
    enum class Type { NUM, STRING } type = Type::NUM;
    double valueD = 0.0;
    std::string valueS;
    explicit PluginParam(const std::string& vName, const double& vValue) : name(vName), type(Type::NUM), valueD(vValue) {}
    explicit PluginParam(const std::string& vName, const std::string& vValue) : name(vName), type(Type::NUM), valueS(vValue) {}
};
typedef std::vector<PluginParam> PluginParams;

struct PluginBridge {};

struct PluginModule {
    virtual ~PluginModule() = default;
    virtual bool init(PluginBridge* vBridgePtr) = 0;
    virtual void unit() = 0;
};

typedef std::shared_ptr<PluginModule> PluginModulePtr;
typedef std::weak_ptr<PluginModule> PluginModuleWeak;

enum class PluginModuleType { NONE = 0, SCRIPTING, Count };

struct PluginModuleInfos {
    std::string path;
    std::string label;
    std::map<std::string, std::string> dico;
    PluginModuleType type;
    std::array<float, 4> color{};
    PluginModuleInfos(const std::string& vPath, const std::string& vLabel, const PluginModuleType& vType, const std::array<float, 4>& vColor = {})
        : path(vPath), label(vLabel), type(vType), color(vColor) {}
};

typedef std::string ScriptFilePathName;

struct ScriptingError {
    ScriptFilePathName file;
    size_t line = 0;
    size_t column = 0;
    std::string message;  // full error text, used as the per-line marker tooltip in the editor
};
typedef std::vector<ScriptingError> ErrorContainer;

// chunk name passed to the scripting runtime when compiling the in-memory project script.
// the host's CodePane uses the SAME string as the sheet id, so a ScriptingError's `file` field
// can be routed back to the matching sheet. keep both sides in sync.
static constexpr const char* sc_PROJECT_SCRIPT_CHUNK = "<project script>";

// lua_register(lua_state_ptr, "print", lua_int_print_args);
struct IDatasModel {
    // add a signal tag with date, color a name. the help will be displayed when mouse over the tag
    // rgba are normalized [0.0:1.0]
    virtual void addSignalTag(double vEpoch, double r, double g, double b, double a, const std::string& vName, const std::string& vHelp) = 0;
    // will add a signal string status
    virtual void addSignalStatus(const std::string& vCategory, const std::string& vName, double vEpoch, const std::string& vStatus) = 0;
    // will add a signal numerical value, the desc is related to the value and will be shown only over the graph
    virtual void addSignalValue(const std::string& vCategory, const std::string& vName, double vEpoch, double vValue, const std::string& vDesc) = 0;
    // will add a signal start zone
    virtual void addSignalStartZone(const std::string& vCategory, const std::string& vName, double vEpoch, const std::string& vStartMsg) = 0;
    // will add a signal end zone
    virtual void addSignalEndZone(const std::string& vCategory, const std::string& vName, double vEpoch, const std::string& vEndMsg) = 0;
};

typedef std::shared_ptr<IDatasModel> IDatasModelPtr;
typedef std::weak_ptr<IDatasModel> IDatasModelWeak;

struct ScriptingDatas {
    std::string buffer;
};
typedef std::string ScriptingModuleName;
struct ScriptingModule : public PluginModule, public IScriptDebugger {
    virtual ~ScriptingModule() = default;
    // will load the related scripting engine
    virtual bool load(IDatasModelWeak vDatasModel) = 0;
    // will unload the related scripting engine
    virtual void unload() = 0;
    // will compile the script from a file path and return errors
    virtual bool compileScript(const ScriptFilePathName& vFilePathName, ErrorContainer& vOutErrors) = 0;
    // will compile the script from in-memory code (project script stored in the .ltg db).
    // default no-op so plugins that only support file-based scripts compile unchanged.
    virtual bool compileScriptCode(const std::string& aCode, ErrorContainer& aOutErrors) {
        (void)aCode;
        (void)aOutErrors;
        return false;
    }
    // will call the start function from script and return errors
    virtual bool callScriptStart(ErrorContainer& vOutErrors) = 0;
    // will call the exec function from script with a buffer and return errors
    virtual bool callScriptExec(const ScriptingDatas& vOutDatas, ErrorContainer& vErrors) = 0;
    // will call the end function from script and return errors
    virtual bool callScriptEnd(ErrorContainer& vOutErrors) = 0;
    // will set the row index
    virtual void setRowIndex(int32_t vRowIndex) = 0;
    // will set the row count
    virtual void setRowCount(int32_t vRowCount) = 0;
};

typedef std::shared_ptr<ScriptingModule> ScriptingModulePtr;
typedef std::weak_ptr<ScriptingModule> ScriptingModuleWeak;

struct PluginInterface {
    virtual ~PluginInterface() = default;
    virtual bool init(ez::Log* vLoggerInstancePtr) = 0;
    virtual void unit() = 0;
    virtual uint32_t getMinimalAppVersionSupported() const = 0;
    virtual uint32_t getVersionMajor() const = 0;
    virtual uint32_t getVersionMinor() const = 0;
    virtual uint32_t getVersionBuild() const = 0;
    virtual std::string getName() const = 0;
    virtual std::string getAuthor() const = 0;
    virtual std::string getVersion() const = 0;
    virtual std::string getContact() const = 0;
    virtual std::string getDescription() const = 0;
    virtual std::vector<PluginModuleInfos> getModulesInfos() const = 0;
    virtual PluginModulePtr createModule(const std::string& vPluginModuleName, Ltg::PluginBridge* vBridgePtr) = 0;
};

}  // namespace Ltg

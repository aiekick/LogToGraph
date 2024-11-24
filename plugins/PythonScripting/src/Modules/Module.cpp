#include "Module.h"

#include <ImGuiPack.h>
#include <EzLibs/EzFile.hpp>
#include <EzLibs/EzTime.hpp>

#include <pybind11/embed.h>
#include <iostream>
#include <fstream>
#include <string>
#include <memory>


#include <chrono>
#include <ctime>

///////////////////////////////////////////////////
/// CUSTOM PYTHON FUNCTIONS ///////////////////////
///////////////////////////////////////////////////

class Logger {
public:
    void log(const std::string& message) { std::cout << "Log: " << message << std::endl; }
};

class SignalProcessor {
private:
    Logger* logger;

public:
    SignalProcessor(Logger* logger) : logger(logger) {}
    void addSignalValue(int signalId, double value) {
        if (logger) {
            logger->log("Processing signal ID: " + std::to_string(signalId) + " with value: " + std::to_string(value));
        }
        std::cout << "Signal ID: " << signalId << ", Value: " << value << std::endl;
    }
};

class ApplicationContext {
private:
    std::unique_ptr<Logger> logger;
    std::unique_ptr<SignalProcessor> processor;

public:
    ApplicationContext() : logger(std::make_unique<Logger>()), processor(std::make_unique<SignalProcessor>(logger.get())) {}

    SignalProcessor* getProcessor() { return processor.get(); }

    Logger* getLogger() { return logger.get(); }
};

namespace py = pybind11;

PYBIND11_MODULE(signal_interface, m) {
    py::class_<Logger>(m, "Logger").def(py::init<>()).def("log", &Logger::log);

    py::class_<SignalProcessor>(m, "SignalProcessor").def(py::init<Logger*>()).def("addSignalValue", &SignalProcessor::addSignalValue);

    py::class_<ApplicationContext>(m, "ApplicationContext")
        .def(py::init<>())
        .def("getProcessor", &ApplicationContext::getProcessor, py::return_value_policy::reference)
        .def("getLogger", &ApplicationContext::getLogger, py::return_value_policy::reference);
}

///////////////////////////////////////////////////
///////////////////////////////////////////////////
///////////////////////////////////////////////////

Ltg::ScriptingModulePtr Module::create(const SettingsWeak& vSettings) {
    assert(!vSettings.expired());
    auto res = std::make_shared<Module>();
    res->m_Settings = vSettings;
    if (!res->init()) {
        res.reset();
    }
    return res;
}

bool Module::init(Ltg::PluginBridge* vBridgePtr) {
    return true;
}

void Module::unit() {
}

bool Module::load() {
    py::scoped_interpreter guard{};  // Démarrer l'interpréteur Python

    // Créer une instance du contexte
    ApplicationContext context;

    // Importer le script Python
    py::module script = py::module::import("script_name");

    // Ouvrir le fichier log
    std::ifstream logFile("log.txt");
    if (!logFile.is_open()) {
        std::cerr << "Erreur : impossible d'ouvrir le fichier." << std::endl;
        return 1;
    }

    std::string line;
    while (std::getline(logFile, line)) {
        // Appeler le script Python pour traiter chaque ligne
        script.attr("process_line")(line, &context);
    }

    return true;
}

void Module::unload() {
}

bool Module::compileScript(const Ltg::ScriptFilePathName& vFilePathName, Ltg::ErrorContainer& vOutErrors) {
    return true;
}

bool Module::callScriptInit(Ltg::ErrorContainer& vOutErrors) {
    return true;
}

bool Module::callScriptStart(Ltg::ErrorContainer& vOutErrors) {
    return true;
}

bool Module::callScriptExec(const Ltg::ScriptingDatas& vOutDatas, Ltg::ErrorContainer& vErrors) {
    return true;
}

bool Module::callScriptEnd(Ltg::ErrorContainer& vOutErrors) {
    return true;
}

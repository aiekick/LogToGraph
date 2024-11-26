#include "Module.h"

#include <ezlibs/ezFile.hpp>
#include <ezlibs/ezTime.hpp>
#include <ImGuiPack.h>

#include <Python.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <mutex>
#include <ctime>

///////////////////////////////////////////////////
/// CUSTOM PYTHON FUNCTIONS ///////////////////////
///////////////////////////////////////////////////

// Singleton pour g�rer les donn�es du mod�le
class Model {
public:
    static Model& getInstance() {
        static Model instance;  // Singleton
        return instance;
    }

    void addSignal(double value) {
        std::lock_guard<std::mutex> lock(mutex_);
        signals_.push_back(value);
        std::cout << "Signal added to model: " << value << std::endl;
    }

    const std::vector<double>& getSignals() const { return signals_; }

private:
    Model() = default;
    ~Model() = default;

    Model(const Model&) = delete;
    Model& operator=(const Model&) = delete;

    std::vector<double> signals_;
    mutable std::mutex mutex_;
};

// Fonction C++ appel�e par Python pour ajouter un signal
PyObject* addSignalValue(PyObject* self, PyObject* args) {
    double value;
    if (!PyArg_ParseTuple(args, "d", &value)) {
        return nullptr;
    }

    // Ajouter la valeur au mod�le via le singleton
    Model::getInstance().addSignal(value);

    Py_RETURN_NONE;
}

// M�thodes Python expos�es
static PyMethodDef SignalMethods[] = {
    {"addSignalValue", addSignalValue, METH_VARARGS, "Add a signal value to the model"},
    {nullptr, nullptr, 0, nullptr}  // Fin
};

// D�finition du module Python
static struct PyModuleDef SignalModule = {
    PyModuleDef_HEAD_INIT,
    "signal_handler",  // Nom du module
    nullptr,           // Documentation
    -1,                // Taille de l'�tat global (pas utilis� ici)
    SignalMethods      // Tableau de fonctions expos�es
};

// Initialisation du module Python
PyMODINIT_FUNC PyInit_signal_handler(void) {
    return PyModule_Create(&SignalModule);
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
    try {
        // Initialisation de Python
        PyImport_AppendInittab("signal_handler", &PyInit_signal_handler);
        Py_Initialize();

        // Charger le script Python
        PyObject* pName = PyUnicode_FromString("script");  // Nom du script Python
        PyObject* pModule = PyImport_Import(pName);
        Py_DECREF(pName);

        if (pModule == nullptr) {
            PyErr_Print();
            std::cerr << "Failed to load 'parser.py'" << std::endl;
            Py_Finalize();
            return 1;
        }

        // R�cup�rer la fonction parse_line
        PyObject* pFunc = PyObject_GetAttrString(pModule, "parse_line");
        if (!pFunc || !PyCallable_Check(pFunc)) {
            if (PyErr_Occurred())
                PyErr_Print();
            std::cerr << "Cannot find function 'parse_line'" << std::endl;
            Py_XDECREF(pFunc);
            Py_DECREF(pModule);
            Py_Finalize();
            return 1;
        }

        // Exemples de lignes � parser
        std::vector<std::string> lines = {"TestConcurrency::test[19:11:43.266341] TEST_VALGRIND INFO_1   SE_IAV_TestConcurrency.cpp::setUp(): Creation of objects",
                                          "[19:11:45.343944] CSC_SE_IAV DEBUG    SE_IAV_Supervisor.cpp::_updateCrIr(): getInitEnCours[1] getInitCompEnCours[1]"};

        for (const std::string& line : lines) {
            // Passer la ligne au script Python
            PyObject* pArgs = PyTuple_Pack(1, PyUnicode_FromString(line.c_str()));
            PyObject* pValue = PyObject_CallObject(pFunc, pArgs);
            Py_DECREF(pArgs);

            if (pValue == nullptr) {
                PyErr_Print();
                std::cerr << "Failed to call Python function" << std::endl;
            } else {
                Py_DECREF(pValue);
            }
        }

        // Afficher les signaux stock�s dans le mod�le
        const auto& signals = Model::getInstance().getSignals();
        std::cout << "Final signals in model:" << std::endl;
        for (double signal : signals) {
            std::cout << signal << std::endl;
        }

        // Nettoyage
        Py_XDECREF(pFunc);
        Py_DECREF(pModule);
        Py_Finalize();

    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
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

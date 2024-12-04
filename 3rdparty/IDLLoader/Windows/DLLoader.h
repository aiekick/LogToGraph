#pragma once

#ifdef WIN32
#include <iostream>
#include <ezlibs/ezLog.hpp>
#include <IDLLoader/IDLLoader.h>
#include "Windows.h"

namespace dlloader {
template <class T>
class DLLoader : public IDLLoader<T> {
private:
    HMODULE _handle = nullptr;
    std::string _pathToLib;
    std::string _allocClassSymbol;
    std::string _deleteClassSymbol;
    bool _isAPlugin = true;

public:
    DLLoader() = default;
    DLLoader(std::string const &pathToLib,
        std::string const &allocClassSymbol = "allocator",
        std::string const &deleteClassSymbol = "deleter")
        : _handle(nullptr),
          _pathToLib(pathToLib),
          _allocClassSymbol(allocClassSymbol),
          _deleteClassSymbol(deleteClassSymbol) {}

    ~DLLoader() = default;

    bool IsValid() const override { return _handle != nullptr; }

    bool IsAPlugin() const override { return _isAPlugin; }

    void DLOpenLib() override {
        _handle = LoadLibraryExA(_pathToLib.c_str(), NULL,  //
            LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
        if (_handle == nullptr) {
            const auto &dw = GetLastError();
            if (dw == ERROR_MOD_NOT_FOUND) {
                LogVarError("Can't open and load %s with error : ERROR_MOD_NOT_FOUND", _pathToLib.c_str());
            } else {
                LogVarError("Can't open and load %s", _pathToLib.c_str());
            }
        }
    }

    std::shared_ptr<T> DLGetInstance() override {
        using allocClass = T *(*)();
        using deleteClass = void (*)(T *);
        if (_handle != nullptr) {
            auto allocFunc = reinterpret_cast<allocClass>(GetProcAddress(_handle, _allocClassSymbol.c_str()));
            auto deleteFunc = reinterpret_cast<deleteClass>(GetProcAddress(_handle, _deleteClassSymbol.c_str()));
            if (!allocFunc || !deleteFunc) {
                _isAPlugin = false;
                DLCloseLib();
                //LogVarDebugError("Can't find allocator or deleter symbol in %s", _pathToLib.c_str());
                return nullptr;
            }
            return std::shared_ptr<T>(allocFunc(), [deleteFunc](T *p) { deleteFunc(p); });
        }
        return nullptr;
    }

    void DLCloseLib() override {
        if (_handle != nullptr && FreeLibrary(_handle) == 0) {
            //LogVarDebugError("Can't close %s", _pathToLib.c_str());
        }
    }
};

}  // namespace dlloader
#endif
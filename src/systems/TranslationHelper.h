/*
Copyright 2021-2023 Stephane Cuillerdier (aka aiekick)

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

#include <ezlibs/ezXmlConfig.hpp>
#include <string>

enum class LanguageEnum { FR = 0, EN };

class TranslationHelper : public ez::xml::Config {
public:
    static LanguageEnum s_HelpLanguage;

public:  // labels
    static const char* layout_menu_name;
    static const char* layout_menu_help;

    static const char* mainframe_menubar_project;
    static const char* mainframe_menubar_project_open;
    static const char* mainframe_menubar_project_reload;
    static const char* mainframe_menubar_project_close;
    static const char* mainframe_menubar_settings;

public:
    void DefineLanguage(LanguageEnum vLanguage, bool vForce = false);
    float DrawMenu();

private:
    void DefineLanguageEN();
    void DefineLanguageFR();

public:  // configuration
    ez::xml::Nodes getXmlNodes(const std::string& vUserDatas = "") final;
    bool setFromXmlNodes(const ez::xml::Node& vNode, const ez::xml::Node& vParent, const std::string& vUserDatas) final;

public:
    static TranslationHelper* Instance() {
        static TranslationHelper _instance;
        return &_instance;
    }

protected:
    TranslationHelper();                                                       // Prevent construction
    TranslationHelper(const TranslationHelper&) {};                            // Prevent construction by copying
    TranslationHelper& operator=(const TranslationHelper&) { return *this; };  // Prevent assignment
    ~TranslationHelper() = default;                                            // Prevent unwanted destruction
};

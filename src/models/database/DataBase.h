/*
Copyright 2022-2023 Stephane Cuillerdier (aka aiekick)

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

#include <memory>
#include <string>
#include <functional>
#include <Headers/Globals.h>

struct sqlite3;
class DataBase {
private:
    sqlite3* m_SqliteDB = nullptr;
    std::string m_DataBaseFilePathName = "datas.db3";
    bool m_TransactionStarted = false;
    char* m_LastErrorMsg = nullptr;

public:
    /// <summary>
    /// check if the file is a valid Sqlite3 DB
    /// </summary>
    /// <param name="vDBFilePathName"></param>
    /// <returns></returns>
    bool IsFileASqlite3DB(const DBFile& vDBFilePathName);

    /// <summary>
    /// will crate a db file (and the tables)
    /// </summary>
    /// <param name="vDBFilePathName"></param>
    /// <returns>trus is sucessfully created</returns>
    bool CreateDBFile(const DBFile& vDBFilePathName);

    /// <summary>
    /// will open a dbfile
    /// </summary>
    /// <param name="vDBFilePathName"></param>
    /// <returns>trus is sucessfully opened</returns>
    bool OpenDBFile(const DBFile& vDBFilePathName);

    /// <summary>
    /// will close the DB File
    /// </summary>
    void CloseDBFile();

    /// <summary>
    /// will begin the transaction mode
    /// </summary>
    /// <returns>trus is sucessfully started</returns>
    bool BeginTransaction();

    /// <summary>
    /// will commit the transaction mode
    /// </summary>
    void CommitTransaction();

    /// <summary>
    /// will cancel the transaction mode
    /// </summary>
    void RollbackTransaction();

    /// <summary>
    /// Add a source file in database
    /// </summary>
    /// <param name="vSourceFile"></param>
    /// <returns>return the id of the entry, or 0</returns>
    DBRowID AddSourceFile(const SourceFileName& vSourceFile);

    /// <summary>
    /// Add a signal category in database
    /// </summary>
    /// <param name="vSignalCategory"></param>
    void AddSignalCategory(const SignalCategory& vSignalCategory);

    /// <summary>
    /// Add a signal name in database
    /// </summary>
    /// <param name="vSignalName"></param>
    void AddSignalName(const SignalName& vSignalName);

    /// <summary>
    /// add a signal tick in database
    /// will also add source file, signal category and signal name
    /// </summary>
    /// <param name="vSourceFile"></param>
    /// <param name="vSignalCategory"></param>
    /// <param name="vName"></param>
    /// <param name="vDate"></param>
    /// <param name="vValue"></param>
    void AddSignalTick(const SourceFileID& vSourceFileID,
                       const SignalCategory& vSignalCategory,
                       const SignalName& vSignalName,
                       const SignalEpochTime& vDate,
                       const SignalValue& vValue);

    /// <summary>
    /// add a signal Status in database with value of type string
    /// will also add source file, signal category and signal name
    /// </summary>
    /// <param name="vSourceFile"></param>
    /// <param name="vSignalCategory"></param>
    /// <param name="vName"></param>
    /// <param name="vDate"></param>
    /// <param name="vValue"></param>
    void AddSignalStatus(const SourceFileID& vSourceFileID,
                         const SignalCategory& vSignalCategory,
                         const SignalName& vSignalName,
                         const SignalEpochTime& vDate,
                         const SignalString& vString,
                         const SignalStatus& vStatus);

    /// <summary>
    /// add a signal tag in database
    /// </summary>
    /// <param name="vDate"></param>
    /// <param name="vColor"></param>
    /// <param name="vName"></param>
    /// <param name="vHelp"></param>
    void AddSignalTag(const SignalEpochTime& vDate, const SignalTagColor& vColor, const SignalTagName& vName, const SignalTagHelp& vHelp);

    /// <summary>
    /// Get the id of a source file from database
    /// </summary>
    /// <param name="vSourceFile"></param>
    /// <returns>return the id of the entry, or 0</returns>
    DBRowID GetSourceFile(const SourceFileName& vSourceFile);

    /// <summary>
    /// Get the id of a signal category from database
    /// </summary>
    /// <param name="vSignalCategory"></param>
    /// <returns>return the id of the entry, or 0</returns>
    DBRowID GetSignalCategory(const SignalCategory& vSignalCategory);

    /// <summary>
    /// Get the id of a signal name from database
    /// </summary>
    /// <param name="vSignalName"></param>
    /// <returns>return the id of the entry, or 0</returns>
    DBRowID GetSignalName(const SignalName& vSignalName);

    /// <summary>
    /// will clear datas tables
    /// </summary>
    void ClearDataTables();

    /// <summary>
    /// willreturn soruce files infos
    /// <param name="vCallback">callback func called for each database line retrieved</param>
    /// </summary>
    void GetSourceFiles(std::function<void(const SourceFileID&, const SourceFilePathName&)> vCallback);

    /// <summary>
    /// will return merged datas in callbakk
    /// <param name="vCallback">callback func called for each database line retrieved</param>
    /// </summary>
    void GetDatas(
        std::function<
            void(const SourceFileID&, const SignalEpochTime&, const SignalCategory&, const SignalName&, const SignalValue&, const SignalString&, const SignalStatus&)>
            vCallback);

    /// <summary>
    /// will return tags in callbakk
    /// <param name="vCallback">callback func called for each database line retrieved</param>
    /// </summary>
    void GetTags(std::function<void(const SignalEpochTime&, const SignalTagColor&, const SignalTagName&, const SignalTagHelp&)> vCallback);

    /// <summary>
    /// return last error message
    /// </summary>
    /// <returns></returns>
    std::string GetLastErrorMesg();

    /// <summary>
    /// will put the xml save in db
    /// </summary>
    /// <param name="vXMLDatas"></param>
    /// <returns>true, is successfully saved</returns>
    bool SetSettingsXMLDatas(const std::string& vXMLDatas);

    /// <summary>
    /// will get from db a settings xml
    /// </summary>
    /// <returns>settings xml datas</returns>
    std::string GetSettingsXMLDatas();

private:
    bool OpenDB();
    void CloseDB();
    bool CreateDB();
    void CreateDBTables();

    /// <summary>
    /// enable foreign key (must be done at each connections)
    /// </summary>
    void EnableForeignKey();

public:  // singleton
    static std::shared_ptr<DataBase> Instance() {
        static std::shared_ptr<DataBase> _instance = std::make_shared<DataBase>();
        return _instance;
    }

public:
    DataBase() = default;                                    // Prevent construction
    DataBase(const DataBase&) = delete;                      // Prevent construction by copying
    DataBase& operator=(const DataBase&) { return *this; };  // Prevent assignment
    virtual ~DataBase() = default;                           // Prevent unwanted destruction};
};
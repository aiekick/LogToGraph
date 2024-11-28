// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

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

#include "DataBase.h"

#include <vector>
#include <sstream>
#include <fstream>
#include <string.h>
#include <sqlite3.hpp>
#include <ezlibs/ezFile.hpp>

// will check database header magic number
// https://www.sqlite.org/fileformat.html : section 1.3
// Offset	Size	Description
// 0	    16	    The header string : "SQLite format 3\000"
bool DataBase::IsFileASqlite3DB(const DBFile& vDBFilePathName) {
    bool res = false;

    std::ifstream file_stream(vDBFilePathName, std::ios_base::binary);
    if (file_stream.is_open()) {
        char magic_header[16 + 1];
        file_stream.read(magic_header, 16U);
        if (strcmp(magic_header, "SQLite format 3\000") == 0) {
            res = true;
        }

        file_stream.close();
    }

    return res;
}

bool DataBase::CreateDBFile(const DBFile& vDBFilePathName) {
    if (!vDBFilePathName.empty()) {
        m_DataBaseFilePathName = vDBFilePathName;
        return CreateDB();
    }
    return false;
}

bool DataBase::OpenDBFile(const DBFile& vDBFilePathName) {
    if (!m_SqliteDB) {
        m_DataBaseFilePathName = vDBFilePathName;
        return OpenDB();
    } else {
        LogVarInfo("%s", "Database already opened\n");
    }
    return (m_SqliteDB != nullptr);
}

void DataBase::CloseDBFile() {
    CloseDB();
}

bool DataBase::BeginTransaction() {
    if (OpenDB()) {
        if (sqlite3_exec(m_SqliteDB, "begin transaction;", nullptr, nullptr, &m_LastErrorMsg) == SQLITE_OK) {
            m_TransactionStarted = true;
            return true;
        }
    }
    LogVarError("Fail to start transaction : %s", m_LastErrorMsg);
    return false;
}

void DataBase::CommitTransaction() {
    if (sqlite3_exec(m_SqliteDB, "COMMIT;", nullptr, nullptr, &m_LastErrorMsg) != SQLITE_OK) {
        LogVarError("Fail to commit : %s", m_LastErrorMsg);
    }
    // we will close the db so force it to reset
    m_TransactionStarted = false;
}

void DataBase::RollbackTransaction() {
    if (sqlite3_exec(m_SqliteDB, "ROLLBACK;", nullptr, nullptr, &m_LastErrorMsg) != SQLITE_OK) {
        LogVarError("Fail to ROLLBACK : %s", m_LastErrorMsg);
    }
    // we will close the db so force it to reset
    m_TransactionStarted = false;
}

void DataBase::EnableForeignKey() {
    // todo : "PRAGMA foreign_keys = ON;"
}

std::string DataBase::GetLastErrorMesg() {
    return std::string(m_LastErrorMsg);
}

bool DataBase::SetSettingsXMLDatas(const std::string& vXMLDatas) {
    if (!vXMLDatas.empty()) {
        std::string insert_query;

        // insert or replace at line 0
        auto xml_datas = GetSettingsXMLDatas();
        if (xml_datas.empty()) {
            insert_query = "insert into app_settings(xml_datas) values(\"" + vXMLDatas + "\");";
        } else {
            insert_query = "update app_settings set xml_datas = \"" + vXMLDatas + "\" where rowid = 1;";
        }

        if (sqlite3_exec(m_SqliteDB, insert_query.c_str(), nullptr, nullptr, &m_LastErrorMsg) != SQLITE_OK) {
#ifdef _DEBUG
            ez::file::saveStringToFile(insert_query, "insert_query.txt");
            ez::file::saveStringToFile(m_LastErrorMsg, "last_error_msg.txt");
#endif
            LogVarError("Fail to insert or replace xml_datas in table app_settings of database : %s", m_LastErrorMsg);
            return false;
        }
        return true;
    }

    return false;
}

std::string DataBase::GetSettingsXMLDatas() {
    std::string res;

    // select at line 0
    auto select_query = u8R"(select * from app_settings where rowid = 1;)";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_SqliteDB, select_query, (int)strlen(select_query), &stmt, nullptr) != SQLITE_OK) {
        LogVarError("%s", "Fail to get xml_datas from app_settings table of database");
    } else {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            auto len = sqlite3_column_bytes(stmt, 0);
            auto txt = (const char*)sqlite3_column_text(stmt, 0);
            if (txt && len) {
                res = std::string(txt, len);
            }
        }
    }

    sqlite3_finalize(stmt);

    return res;
}

DBRowID DataBase::AddSourceFile(const SourceFileName& vSourceFile) {
    auto insert_query = ez::str::toStr(u8R"(insert or ignore into signal_sources (source) values("%s");)", vSourceFile.c_str());
    if (sqlite3_exec(m_SqliteDB, insert_query.c_str(), nullptr, nullptr, &m_LastErrorMsg) != SQLITE_OK) {
        LogVarError("Fail to insert a source file in database : %s", m_LastErrorMsg);
        return -1;
    }
    return (DBRowID)sqlite3_last_insert_rowid(m_SqliteDB);
}

void DataBase::AddSignalCategory(const SignalCategory& vSignalCategory) {
    auto insert_query = ez::str::toStr(u8R"(insert or ignore into signal_categories (category) values("%s");)", vSignalCategory.c_str());
    if (sqlite3_exec(m_SqliteDB, insert_query.c_str(), nullptr, nullptr, &m_LastErrorMsg) != SQLITE_OK) {
        LogVarError("Fail to insert a signal category in database : %s", m_LastErrorMsg);
    }
}

void DataBase::AddSignalName(const SignalName& vSignalName) {
    auto insert_query = ez::str::toStr(u8R"(insert or ignore into signal_names (name) values("%s");)", vSignalName.c_str());
    if (sqlite3_exec(m_SqliteDB, insert_query.c_str(), nullptr, nullptr, &m_LastErrorMsg) != SQLITE_OK) {
        LogVarError("Fail to insert a signal name in database : %s", m_LastErrorMsg);
    }
}

void DataBase::AddSignalTick(const SourceFileID& vSourceFileID,
                             const SignalCategory& vSignalCategory,
                             const SignalName& vSignalName,
                             const SignalEpochTime& vDate,
                             const SignalValue& vValue,
                             const SignalDesc& vDesc) {
    AddSignalCategory(vSignalCategory);
    AddSignalName(vSignalName);

    auto insert_query = ez::str::toStr(
        u8R"(insert or ignore into signal_ticks 
(id_signal_source, id_signal_category, id_signal_name, epoch_time, signal_value, signal_desc) values(%i,
(select rowid from signal_categories where signal_categories.category = "%s"),
(select rowid from signal_names where signal_names.name = "%s"),
%f,%f,"%s");)",
        (int32_t)vSourceFileID,
        vSignalCategory.c_str(),
        vSignalName.c_str(),
        vDate,
        vValue,
        vDesc.c_str());
    if (sqlite3_exec(m_SqliteDB, insert_query.c_str(), nullptr, nullptr, &m_LastErrorMsg) != SQLITE_OK) {
        LogVarError("Fail to insert a tick in database : %s", m_LastErrorMsg);
    }
}

void DataBase::AddSignalStatus(const SourceFileID& vSourceFileID,
                               const SignalCategory& vSignalCategory,
                               const SignalName& vSignalName,
                               const SignalEpochTime& vDate,
                               const SignalString& vString,
                               const SignalStatus& vStatus) {
    AddSignalCategory(vSignalCategory);
    AddSignalName(vSignalName);

    auto insert_query = ez::str::toStr(
        u8R"(insert or ignore into signal_ticks 
(id_signal_source, id_signal_category, id_signal_name, epoch_time, signal_string, signal_status) values(%i,
(select rowid from signal_categories where signal_categories.category = "%s"),
(select rowid from signal_names where signal_names.name = "%s"),
%f,"%s","%s");)",
        (int32_t)vSourceFileID,
        vSignalCategory.c_str(),
        vSignalName.c_str(),
        vDate,
        vString.c_str(),
        vStatus.c_str());
    if (sqlite3_exec(m_SqliteDB, insert_query.c_str(), nullptr, nullptr, &m_LastErrorMsg) != SQLITE_OK) {
        LogVarError("Fail to insert a tick in database : %s", m_LastErrorMsg);
    }
}

void DataBase::AddSignalTag(const SignalEpochTime& vDate, const SignalTagColor& vColor, const SignalTagName& vName, const SignalTagHelp& vHelp) {
    auto insert_query = ez::str::toStr(
        u8R"(insert or ignore into signal_tags 
(epoch_time, tag_color, tag_name, tag_help) values (%f, "%u;%u;%u;%u", "%s", "%s");)",
        vDate,
        (uint32_t)vColor.x,
        (uint32_t)vColor.y,
        (uint32_t)vColor.z,
        (uint32_t)vColor.w,
        vName.c_str(),
        vHelp.c_str());
    if (sqlite3_exec(m_SqliteDB, insert_query.c_str(), nullptr, nullptr, &m_LastErrorMsg) != SQLITE_OK) {
        LogVarError("Fail to insert a tag in database : %s", m_LastErrorMsg);
    }
}

DBRowID DataBase::GetSourceFile(const SourceFileName& vSourceFile) {
    DBRowID res = -1;

    auto select_query = ez::str::toStr(u8R"(select rowid from signal_sources where signal_sources.source = "%s";)", vSourceFile.c_str());
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_SqliteDB, select_query.c_str(), (int)select_query.size(), &stmt, nullptr) != SQLITE_OK) {
        LogVarError("%s", "Fail to get id from signal_sources in database");
    } else {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            res = (DBRowID)sqlite3_column_int(stmt, 0);
        }
    }

    sqlite3_finalize(stmt);

    return res;
}

DBRowID DataBase::GetSignalCategory(const SignalCategory& vSignalCategory) {
    DBRowID res = -1;

    auto select_query = ez::str::toStr(u8R"(select rowid from signal_categories where signal_categories.category = "%s";)", vSignalCategory.c_str());
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_SqliteDB, select_query.c_str(), (int)select_query.size(), &stmt, nullptr) != SQLITE_OK) {
        LogVarError("%s", "Fail to get id from signal_categories in database");
    } else {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            res = (DBRowID)sqlite3_column_int(stmt, 0);
        }
    }

    sqlite3_finalize(stmt);

    return res;
}

DBRowID DataBase::GetSignalName(const SignalName& vSignalName) {
    DBRowID res = -1;

    auto select_query = ez::str::toStr(u8R"(select rowid from signal_names where signal_names.name = "%s";)", vSignalName.c_str());
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_SqliteDB, select_query.c_str(), (int)select_query.size(), &stmt, nullptr) != SQLITE_OK) {
        LogVarError("%s", "Fail to get id from signal_names in database");
    } else {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            res = (DBRowID)sqlite3_column_int(stmt, 0);
        }
    }

    sqlite3_finalize(stmt);

    return res;
}

void DataBase::ClearDataTables() {
    auto clear_query =
        u8R"(
begin transaction;
delete from signal_sources;
delete from signal_categories;
delete from signal_names;
delete from signal_ticks;
delete from signal_tags;
commit;
)";
    if (sqlite3_exec(m_SqliteDB, clear_query, nullptr, nullptr, &m_LastErrorMsg) != SQLITE_OK) {
        LogVarError("Fail to clear datas tables in database : %s", m_LastErrorMsg);
    }
}

void DataBase::GetSourceFiles(std::function<void(const SourceFileID&, const SourceFilePathName&)> vCallback) {
    // no interest to call that without a callback for retrieve datas
    assert(vCallback);

    std::string select_query =
        u8R"(
SELECT
  signal_sources.rowid as source_id,
  signal_sources.source as source_name
FROM 
 signal_sources
ORDER BY
 source_id
;
)";
    sqlite3_stmt* stmt = nullptr;
    int res = sqlite3_prepare_v2(m_SqliteDB, select_query.c_str(), (int)select_query.size(), &stmt, nullptr);
    if (res != SQLITE_OK) {
        LogVarError("%s", "Fail to get id from signal_names in database");
    } else {
        while (res == SQLITE_OK || res == SQLITE_ROW) {
            // on r�cup�re une ligne dans la table
            res = sqlite3_step(stmt);
            if (res == SQLITE_OK || res == SQLITE_ROW) {
                /*
                    signal_sources.rowid	int
                    signal_sources.source	string
                */
                auto source_file_id = sqlite3_column_int(stmt, 0);

                // can be null
                auto source_file_path_name_cstr = (const char*)sqlite3_column_text(stmt, 1);
                std::string source_file_path_name_string = (source_file_path_name_cstr != nullptr) ? source_file_path_name_cstr : "";

                // call callback with datas passed in args
                vCallback(source_file_id, source_file_path_name_string);
            }
        }
    }

    sqlite3_finalize(stmt);
}

void DataBase::GetDatas(std::function<void(const SourceFileID&,
                                           const SignalEpochTime&,
                                           const SignalCategory&,
                                           const SignalName&,
                                           const SignalValue&,
                                           const SignalString&,
                                           const SignalStatus&,
                                           const SignalDesc&)> vCallback) {
    // no interest to call that without a callback for retrieve datas
    assert(vCallback);

    std::string select_query =
        u8R"(
SELECT
  signal_sources.rowid as source_id,
  signal_ticks.epoch_time as epoch_time,
  signal_categories.category as category,
  signal_names.name as name, 
  signal_ticks.signal_value as value,
  signal_ticks.signal_string as string,
  signal_ticks.signal_status as string,
  signal_ticks.signal_desc as string
FROM 
 signal_ticks
 LEFT JOIN signal_sources ON signal_ticks.id_signal_source = signal_sources.rowid
 LEFT JOIN signal_categories ON signal_ticks.id_signal_category = signal_categories.rowid
 LEFT JOIN signal_names ON signal_ticks.id_signal_name = signal_names.rowid
order by 
 epoch_time
;
)";
    sqlite3_stmt* stmt = nullptr;
    int res = sqlite3_prepare_v2(m_SqliteDB, select_query.c_str(), (int)select_query.size(), &stmt, nullptr);
    if (res != SQLITE_OK) {
        LogVarError("%s", "Fail to get id from signal_names in database");
    } else {
        while (res == SQLITE_OK || res == SQLITE_ROW) {
            // on r�cup�re une ligne dans la table
            res = sqlite3_step(stmt);
            if (res == SQLITE_OK || res == SQLITE_ROW) {
                /*
                  signal_sources.rowid		    uint
                  signal_ticks.epoch_time		double
                  signal_categories.category    string
                  signal_names.name				string
                  signal_ticks.signal_value		double
                  signal_ticks.signal_string	string
                  signal_ticks.signal_desc	    string
                */

                auto source_file_id = sqlite3_column_int(stmt, 0);
                auto epoch_time = sqlite3_column_double(stmt, 1);

                // can be null
                auto category_cstr = (const char*)sqlite3_column_text(stmt, 2);
                std::string category_string = (category_cstr != nullptr) ? category_cstr : "";

                // can be null
                auto name_cstr = (const char*)sqlite3_column_text(stmt, 3);
                std::string name_string = (name_cstr != nullptr) ? name_cstr : "";

                auto signal_value = sqlite3_column_double(stmt, 4);

                // can be null
                auto signal_string_cstr = (const char*)sqlite3_column_text(stmt, 5);
                std::string signal_string = (signal_string_cstr != nullptr) ? signal_string_cstr : "";

                // can be null
                auto signal_status_cstr = (const char*)sqlite3_column_text(stmt, 6);
                std::string signal_status = (signal_status_cstr != nullptr) ? signal_status_cstr : "";

                // can be null
                auto signal_desc_cstr = (const char*)sqlite3_column_text(stmt, 7);
                std::string signal_desc = (signal_desc_cstr != nullptr) ? signal_desc_cstr : "";

                // call callback with datas passed in args
                vCallback(source_file_id, epoch_time, category_string, name_string, signal_value, signal_string, signal_status, signal_desc);
            }
        }
    }

    sqlite3_finalize(stmt);
}

void DataBase::GetTags(std::function<void(const SignalEpochTime&, const SignalTagColor&, const SignalTagName&, const SignalTagHelp&)> vCallback) {
    // no interest to call that without a callback for retrieve datas
    assert(vCallback);

    std::string select_query =
        u8R"(
SELECT
  signal_tags.epoch_time as epoch_time,
  signal_tags.tag_color as color,
  signal_tags.tag_name as name,
  signal_tags.tag_help as help
FROM 
 signal_tags
;
)";
    sqlite3_stmt* stmt = nullptr;
    int res = sqlite3_prepare_v2(m_SqliteDB, select_query.c_str(), (int)select_query.size(), &stmt, nullptr);
    if (res != SQLITE_OK) {
        LogVarError("%s", "Fail to get id from signal_tags in database");
    } else {
        while (res == SQLITE_OK || res == SQLITE_ROW) {
            // on r�cup�re une ligne dans la table
            res = sqlite3_step(stmt);
            if (res == SQLITE_OK || res == SQLITE_ROW) {
                /*
                  signal_tags.epoch_time        double
                  signal_tags.tag_color         string
                  signal_tags.tag_name          string
                  signal_tags.tag_help          string
                */

                auto epoch_time = sqlite3_column_double(stmt, 0);

                // can be null
                SignalTagColor tag_color;
                auto tag_color_cstr = (const char*)sqlite3_column_text(stmt, 1);
                std::string tag_color_string = (tag_color_cstr != nullptr) ? tag_color_cstr : "";
                if (!tag_color_string.empty()) {
                    tag_color = ez::fvariant(tag_color_string).GetV4(';') / 255.0f;
                }

                // can be null
                auto tag_name_cstr = (const char*)sqlite3_column_text(stmt, 2);
                std::string tag_name_string = (tag_name_cstr != nullptr) ? tag_name_cstr : "";

                // can be null
                auto tag_help_cstr = (const char*)sqlite3_column_text(stmt, 3);
                std::string tag_help_string = (tag_help_cstr != nullptr) ? tag_help_cstr : "";

                // call callback with datas passed in args
                vCallback(epoch_time, tag_color, tag_name_string, tag_help_string);
            }
        }
    }

    sqlite3_finalize(stmt);
}

////////////////////////////////////////////////////////////
///// PRIVATE //////////////////////////////////////////////
////////////////////////////////////////////////////////////

bool DataBase::OpenDB() {
    if (!m_SqliteDB) {
        if (sqlite3_open_v2(m_DataBaseFilePathName.c_str(), &m_SqliteDB, SQLITE_OPEN_READWRITE, nullptr) != SQLITE_OK)  // db possibily not exist
        {
            CreateDBTables();
        }
    }

    return (m_SqliteDB != nullptr);
}

bool DataBase::CreateDB() {
    CloseDB();

    if (!m_SqliteDB) {
        ez::file::destroyFile(m_DataBaseFilePathName);

        if (sqlite3_open_v2(m_DataBaseFilePathName.c_str(), &m_SqliteDB, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr) == SQLITE_OK)  // db possibily not exist
        {
            CreateDBTables();
            CloseDB();
        }
    }

    return (m_SqliteDB != nullptr);
}

void DataBase::CreateDBTables() {
    if (m_SqliteDB)  // in the doubt
    {
        const char* create_tables =
            u8R"(
create table signal_sources (
	source VARCHAR(1024) UNIQUE
);

create table signal_categories (
	category varchar(1024) UNIQUE
);

create table signal_names (
	name varchar(255) UNIQUE
);

create table signal_ticks (
	id_signal_source INTEGER, 
	id_signal_category INTEGER, 
	id_signal_name INTEGER,
	epoch_time integer, 
	signal_value double, 
	signal_string varchar(255), 
	signal_status varchar(255), 
	signal_desc varchar(255)
);

create table signal_tags (
	epoch_time integer unique, 
	tag_color varchar(20),
	tag_name varchar(255),
	tag_help varchar(1024)
);

create table app_settings (
	xml_datas TEXT
);
)";

        // signal_tags.tag_color is like this format 128;250,100;255

        if (sqlite3_exec(m_SqliteDB, create_tables, nullptr, nullptr, &m_LastErrorMsg) != SQLITE_OK) {
            if (m_LastErrorMsg) {
                LogVarError("Fail to create database : %s", m_LastErrorMsg);
            } else {
                LogVarError("%s", "Fail to create database");
            }
            m_SqliteDB = nullptr;
        }
    }
}

void DataBase::CloseDB() {
    if (m_SqliteDB) {
        if (sqlite3_close(m_SqliteDB) == SQLITE_BUSY) {
            // try to force closing
            sqlite3_close_v2(m_SqliteDB);
        }
    }

    m_SqliteDB = nullptr;

    // there is also sqlite3LeaveMutexAndCloseZombie when sqlite is stucked
}
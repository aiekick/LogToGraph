#include "DBEngine.h"

#include <vector>
#include <sstream>
#include <string.h>  
#include <ctools/cTools.h>
#include <ctools/Logger.h>
#include <sqlite/sqlite3.h>

bool DBEngine::CreateDBFile(const DBFile& vDBFilePathName)
{
	if (!vDBFilePathName.empty())
	{
		m_DataBaseFilePathName = vDBFilePathName;

		return OpenDB();
	}

	return false;
}

bool DBEngine::OpenDBFile(const DBFile& vDBFilePathName)
{
	if (!m_SqliteDB)
	{
		m_DataBaseFilePathName = vDBFilePathName;

		return OpenDB();
	}
	else
	{
		LogVarInfo("Database already opened\n");
	}

	return (m_SqliteDB != nullptr);
}

void DBEngine::CloseDBFile()
{
	CloseDB();
}

bool DBEngine::BeginTransaction()
{
	if (OpenDB())
	{
		if (sqlite3_exec(m_SqliteDB, "begin transaction;", nullptr, nullptr, &m_LastErrorMsg) == SQLITE_OK)
		{
			m_TransactionStarted = true;

			return true;
		}
	}

	LogVarError("Fail to start transaction : %s", m_LastErrorMsg);

	return false;
}

void DBEngine::EndTransaction()
{
	if (sqlite3_exec(m_SqliteDB, "COMMIT;", nullptr, nullptr, &m_LastErrorMsg) != SQLITE_OK)
	{
		LogVarError("Fail to commit : %s", m_LastErrorMsg);
	}

	// we will close the db so force it to reset
	m_TransactionStarted = false;
}

void DBEngine::EnableForeignKey()
{
	//todo : "PRAGMA foreign_keys = ON;"
}

std::string DBEngine::GetLastErrorMesg()
{
	return std::string(m_LastErrorMsg);
}

DBRowID DBEngine::AddSourceFile(const SourceFileName& vSourceFile)
{
	auto insert_query = ct::toStr(u8R"(insert or ignore into signal_sources (source) values("%s");)", vSourceFile.c_str());
	if (sqlite3_exec(m_SqliteDB, insert_query.c_str(), nullptr, nullptr, &m_LastErrorMsg) != SQLITE_OK)
	{
		LogVarError("Fail to insert a source file in database : %s", m_LastErrorMsg);
		return -1;
	}
	return (DBRowID)sqlite3_last_insert_rowid(m_SqliteDB);
}

void DBEngine::AddSignalCategory(const SignalCategory& vSignalCategory)
{
	auto insert_query = ct::toStr(u8R"(insert or ignore into signal_categories (category) values("%s");)", vSignalCategory.c_str());
	if (sqlite3_exec(m_SqliteDB, insert_query.c_str(), nullptr, nullptr, &m_LastErrorMsg) != SQLITE_OK)
	{
		LogVarError("Fail to insert a singal category in database : %s", m_LastErrorMsg);
	}
}

void DBEngine::AddSignalName(const SignalName& vSignalName)
{
	auto insert_query = ct::toStr(u8R"(insert or ignore into signal_names (name) values("%s");)", vSignalName.c_str());
	if (sqlite3_exec(m_SqliteDB, insert_query.c_str(), nullptr, nullptr, &m_LastErrorMsg) != SQLITE_OK)
	{
		LogVarError("Fail to insert a singal name in database : %s", m_LastErrorMsg);
	}
}

void DBEngine::AddSignalTick(const SourceFileID& vSourceFileID, const SignalCategory& vSignalCategory, const SignalName& vSignalName, const SignalEpochTime& vDate, const SignalValue& vValue)
{
	AddSignalCategory(vSignalCategory);
	AddSignalName(vSignalName);

	auto insert_query = ct::toStr(u8R"(insert or ignore into signal_ticks 
(id_signal_source, id_signal_category, id_signal_name, epoch_time, signal_value) values(%i,
(select rowid from signal_categories where signal_categories.category = "%s"),
(select rowid from signal_names where signal_names.name = "%s"),
%f,%f);)",
(int32_t)vSourceFileID, vSignalCategory.c_str(), vSignalName.c_str(), vDate, vValue);
	if (sqlite3_exec(m_SqliteDB, insert_query.c_str(), nullptr, nullptr, &m_LastErrorMsg) != SQLITE_OK)
	{
		LogVarError("Fail to insert a tick in database : %s", m_LastErrorMsg);
	}
}

void DBEngine::AddSignalTick(const SourceFileID& vSourceFileID, const SignalCategory& vSignalCategory, const SignalName& vSignalName, const SignalEpochTime& vDate, const SignalString& vString)
{
	AddSignalCategory(vSignalCategory);
	AddSignalName(vSignalName);

	auto insert_query = ct::toStr(u8R"(insert or ignore into signal_ticks 
(id_signal_source, id_signal_category, id_signal_name, epoch_time, signal_string) values(%i,
(select rowid from signal_categories where signal_categories.category = "%s"),
(select rowid from signal_names where signal_names.name = "%s"),
%f,"%s");)",
(int32_t)vSourceFileID, vSignalCategory.c_str(), vSignalName.c_str(), vDate, vString.c_str());
	if (sqlite3_exec(m_SqliteDB, insert_query.c_str(), nullptr, nullptr, &m_LastErrorMsg) != SQLITE_OK)
	{
		LogVarError("Fail to insert a tick in database : %s", m_LastErrorMsg);
	}
}

DBRowID DBEngine::GetSourceFile(const SourceFileName& vSourceFile)
{
	DBRowID res = -1;

	auto select_query = ct::toStr(u8R"(select rowid from signal_sources where signal_sources.source = "%s";)", vSourceFile.c_str());
	sqlite3_stmt* stmt = nullptr;
	if (sqlite3_prepare_v2(m_SqliteDB, select_query.c_str(), (int)select_query.size(), &stmt, nullptr) != SQLITE_OK)
	{
		LogVarError("Fail to get id from signal_sources in database");
	}
	else
	{
		res = (DBRowID)sqlite3_column_int(stmt, 0);
	}

	sqlite3_finalize(stmt);

	return res;
}

DBRowID DBEngine::GetSignalCategory(const SignalCategory& vSignalCategory)
{
	DBRowID res = -1;

	auto select_query = ct::toStr(u8R"(select rowid from signal_categories where signal_categories.category = "%s";)", vSignalCategory.c_str());
	sqlite3_stmt* stmt = nullptr;
	if (sqlite3_prepare_v2(m_SqliteDB, select_query.c_str(), (int)select_query.size(), &stmt, nullptr) != SQLITE_OK)
	{
		LogVarError("Fail to get id from signal_categories in database");
	}
	else
	{
		res = (DBRowID)sqlite3_column_int(stmt, 0);
	}

	sqlite3_finalize(stmt);

	return res;
}

DBRowID DBEngine::GetSignalName(const SignalName& vSignalName)
{
	DBRowID res = -1;

	auto select_query = ct::toStr(u8R"(select rowid from signal_names where signal_names.name = "%s";)", vSignalName.c_str());
	sqlite3_stmt* stmt = nullptr;
	if (sqlite3_prepare_v2(m_SqliteDB, select_query.c_str(), (int)select_query.size(), &stmt, nullptr) != SQLITE_OK)
	{
		LogVarError("Fail to get id from signal_names in database");
	}
	else
	{
		res = (DBRowID)sqlite3_column_int(stmt, 0);
	}

	sqlite3_finalize(stmt);

	return res;
}

void DBEngine::ClearDataTables()
{
	auto clear_query = u8R"(
begin transaction;
delete from signal_sources;
delete from signal_categories;
delete from signal_names;
delete from signal_ticks;
commit;
)";
	if (sqlite3_exec(m_SqliteDB, clear_query, nullptr, nullptr, &m_LastErrorMsg) != SQLITE_OK)
	{
		LogVarError("Fail to clear datas tables in database : %s", m_LastErrorMsg);
	}
}

void DBEngine::GetSourceFiles(std::function<void(const SourceFileID&, const SourceFilePathName&)> vCallback)
{
	// no interest to call that without a claaback for retrieve datas
	assert(vCallback);

	std::string select_query = u8R"(
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
	if (res != SQLITE_OK)
	{
		LogVarError("Fail to get id from signal_names in database");
	}
	else
	{
		while (res == SQLITE_OK || res == SQLITE_ROW)
		{
			//on récupère une ligne dans la table
			res = sqlite3_step(stmt);
			if (res == SQLITE_OK || res == SQLITE_ROW)
			{
				/*
					signal_sources.rowid	int
					signal_sources.source	string
				*/
				auto source_file_id = sqlite3_column_int(stmt, 0);

				//can be null
				auto source_file_path_name_cstr = (const char*)sqlite3_column_text(stmt, 1);
				std::string source_file_path_name_string = (source_file_path_name_cstr != nullptr) ? source_file_path_name_cstr : "";

				//call callback with datas passed in args
				vCallback(source_file_id, source_file_path_name_string);
			}
		}
	}

	sqlite3_finalize(stmt);
}

void DBEngine::GetDatas(std::function<void(const SourceFileID&, const SignalEpochTime&, const SignalCategory&, const SignalName&, const SignalValue&, const SignalString&)> vCallback)
{
	// no interest to call that without a claaback for retrieve datas
	assert(vCallback);

	std::string select_query = u8R"(
SELECT
  signal_sources.rowid as source_id,
  signal_ticks.epoch_time as epoch_time,
  signal_categories.category as category,
  signal_names.name as name, 
  signal_ticks.signal_value as value,
  signal_ticks.signal_string as string
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
	if (res != SQLITE_OK)
	{
		LogVarError("Fail to get id from signal_names in database");
	}
	else
	{
		while (res == SQLITE_OK || res == SQLITE_ROW)
		{
			//on récupère une ligne dans la table
			res = sqlite3_step(stmt);
			if (res == SQLITE_OK || res == SQLITE_ROW)
			{
				/*				
				  signal_sources.rowid		    uint
				  signal_ticks.epoch_time		double
				  signal_categories.category    string
				  signal_names.name				string
				  signal_ticks.signal_value		double
				  signal_ticks.signal_string	string
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

				//call callback with datas passed in args
				vCallback(source_file_id, epoch_time, category_string, name_string, signal_value, signal_string);
			}
		}
	}

	sqlite3_finalize(stmt);
}

////////////////////////////////////////////////////////////
///// PRIVATE //////////////////////////////////////////////
////////////////////////////////////////////////////////////

bool DBEngine::OpenDB()
{
	if (!m_SqliteDB)
	{
		if (sqlite3_open_v2(m_DataBaseFilePathName.c_str(), &m_SqliteDB, SQLITE_OPEN_READWRITE, nullptr) != SQLITE_OK) // db possibily not exist
		{
			CreateDB();
		}
	}

	return (m_SqliteDB != nullptr);
}

void DBEngine::CreateDB()
{
	m_SqliteDB = nullptr;

	if (sqlite3_open_v2(m_DataBaseFilePathName.c_str(), &m_SqliteDB, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr) == SQLITE_OK)
	{
		if (m_SqliteDB) // in the doubt
		{
			const char* create_tables = u8R"(
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
	signal_string varchar(255)
);
)";

			if (sqlite3_exec(m_SqliteDB, create_tables, nullptr, nullptr, &m_LastErrorMsg) != SQLITE_OK)
			{
				LogVarError("Fail to create database : %s", m_LastErrorMsg);
				m_SqliteDB = nullptr;
			}
		}
	}
	else
	{
		LogVarError("Fail to open or create database\n");
		m_SqliteDB = nullptr;
	}
}

void DBEngine::CloseDB()
{
	if (m_SqliteDB)
	{
		if (sqlite3_close(m_SqliteDB) == SQLITE_BUSY)
		{
			// try to force closing
			sqlite3_close_v2(m_SqliteDB);
		}
	}

	m_SqliteDB = nullptr;

	//there is also sqlite3LeaveMutexAndCloseZombie when sqlite is stucked
}
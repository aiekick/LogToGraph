#pragma once

/*
les tables

table : signal_sources
id, file

table : signal_series
id, id_source, epoch, id_category, id_name, signal value
category pointera sur la table categories
signal name pointe sur la tables signal_names

table signal_categories
id, name

table signal_names
id, name

*/

/*
std::string requetAccountReport = "";
requetAccountReport += "select ";
requetAccountReport += "_op.id as ID, ";
requetAccountReport += "_account.account as Compte, ";
requetAccountReport += "_op.date as Date, ";
requetAccountReport += "_op.desc as Description, ";
requetAccountReport += "_op.memo as Detail, ";
requetAccountReport += "_op.debit as Debit, ";
requetAccountReport += "_op.credit as Credit, ";
requetAccountReport += "_cat.cat as Categorie ";
requetAccountReport += "from _account, _op, _cat ";
requetAccountReport += "where (";
requetAccountReport += "_op.idAccount=_account.id ";
requetAccountReport += "and ";
requetAccountReport += "_op.idAccount=\"" + toStr(accountId) + "\"";
requetAccountReport += " and ";
requetAccountReport += "_op.idCategorie=_cat.id";
requetAccountReport += ")";
requetAccountReport += " order by Date asc";
*/
#include <memory>
#include <string>
#include <Headers/Globals.h>

struct sqlite3;
class DBEngine
{
private:
	sqlite3* m_SqliteDB = nullptr;
	std::string m_DataBaseFilePathName = "datas.db3";
	bool m_TransactionStarted = false;
	char* m_LastErrorMsg = nullptr;

public:
	
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
	/// will start the transaction mode
	/// </summary>
	/// <returns>trus is sucessfully started</returns>
	bool BeginTransaction();
	
	/// <summary>
	/// will end the transaction mode
	/// </summary>
	void EndTransaction();

	/// <summary>
	/// Add a source file in database
	/// </summary>
	/// <param name="vSourceFile"></param>
	/// <returns>return the id of the entry, or 0</returns>
	DBRowID AddSourceFile(const SourceFile& vSourceFile);

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
	void AddSignalTick(const SourceFileID& vSourceFileID, const SignalCategory& vSignalCategory, const SignalName& vSignalName, const SignalEpochTime& vDate, const SignalValue& vValue);
	
	/// <summary>
	/// Get the id of a source file from database
	/// </summary>
	/// <param name="vSourceFile"></param>
	/// <returns>return the id of the entry, or 0</returns>
	DBRowID GetSourceFile(const SourceFile& vSourceFile);

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
	/// return last error message
	/// </summary>
	/// <returns></returns>
	std::string GetLastErrorMesg();

private:
	bool OpenDB();
	void CloseDB();
	void CreateDB();

	/// <summary>
	/// enable foreign key (must be done at each connections)
	/// </summary>
	void EnableForeignKey();

public: // singleton
	static std::shared_ptr<DBEngine> Instance()
	{
		static std::shared_ptr<DBEngine> _instance = std::make_shared<DBEngine>();
		return _instance;
	}

public:
	DBEngine() = default; // Prevent construction
	DBEngine(const DBEngine&) = delete; // Prevent construction by copying
	DBEngine& operator =(const DBEngine&) { return *this; }; // Prevent assignment
	virtual ~DBEngine() = default; // Prevent unwanted destruction};
};
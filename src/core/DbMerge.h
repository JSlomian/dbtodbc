#pragma once

#include "DbcValue.h"
#include "MySqlClient.h"
#include <string>

// Reimplements AzerothCore's DBCDatabaseLoader overwrite-or-append-by-ID semantics:
// DB rows overwrite matching-ID rows in `table`, rows with new IDs are appended.
// Sets table.columnNames from the already-fetched DB schema. Returns false (with
// *outError set) if the column count doesn't match the DBC's format string (the
// invariant AzerothCore itself asserts on).
bool ApplyDbRowsToTable(std::vector<std::string> const& columnNames,
    std::vector<std::vector<std::optional<std::string>>> const& dbRows,
    DbcTable& table, std::string* outError);

// Fetches tableName via db.FetchTable then applies it with ApplyDbRowsToTable.
// Sets table.tableName. Returns false (with *outError set) if the query fails or
// the DB table doesn't exist.
bool MergeDbRowsIntoTable(MySqlClient& db, std::string const& tableName, DbcTable& table, std::string* outError);

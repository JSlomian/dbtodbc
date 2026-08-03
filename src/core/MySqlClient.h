#pragma once

#include <optional>
#include <string>
#include <vector>

// Thin wrapper over the raw libmariadb/libmysqlclient C API (text protocol only -
// this tool only ever does simple SELECT * queries, no transactions or prepared
// statements needed).
class MySqlClient
{
public:
    MySqlClient();
    ~MySqlClient();

    MySqlClient(MySqlClient const&) = delete;
    MySqlClient& operator=(MySqlClient const&) = delete;

    bool Connect(std::string const& host, unsigned port, std::string const& user, std::string const& password, std::string const& database);
    bool TableExists(std::string const& table);

    // SELECT * FROM `table` ORDER BY `ID` ASC - column 0 is always ID by AzerothCore's
    // *_dbc.sql convention. Returns false (with LastError() set) on query failure.
    bool FetchTable(std::string const& table, std::vector<std::string>& outColumnNames, std::vector<std::vector<std::optional<std::string>>>& outRows);

    [[nodiscard]] std::string LastError() const;

private:
    void* _conn; // MYSQL*, kept opaque here so this header doesn't need <mysql.h>
};

#include "MySqlClient.h"
#include <mysql.h>
#include <algorithm>
#include <cstdlib>
#include <cstring>

namespace
{
    MYSQL* AsMysql(void* conn) { return static_cast<MYSQL*>(conn); }
}

MySqlClient::MySqlClient() : _conn(nullptr)
{
}

MySqlClient::~MySqlClient()
{
    if (_conn)
        mysql_close(AsMysql(_conn));
}

bool MySqlClient::Connect(std::string const& host, unsigned port, std::string const& user, std::string const& password, std::string const& database)
{
    MYSQL* conn = mysql_init(nullptr);
    if (!conn)
        return false;

    MYSQL* result = mysql_real_connect(conn, host.c_str(), user.c_str(), password.c_str(),
        database.c_str(), port, nullptr, 0);

    _conn = conn;
    return result != nullptr;
}

bool MySqlClient::TableExists(std::string const& table)
{
    MYSQL* conn = AsMysql(_conn);

    char escaped[512];
    mysql_real_escape_string(conn, escaped, table.c_str(), std::min(table.size(), sizeof(escaped) / 2 - 1));

    std::string query = "SELECT COUNT(*) FROM information_schema.tables WHERE table_schema = DATABASE() AND table_name = '";
    query += escaped;
    query += "'";

    if (mysql_query(conn, query.c_str()) != 0)
        return false;

    MYSQL_RES* res = mysql_store_result(conn);
    if (!res)
        return false;

    bool exists = false;
    if (MYSQL_ROW row = mysql_fetch_row(res))
        exists = row[0] && std::atoi(row[0]) > 0;

    mysql_free_result(res);
    return exists;
}

bool MySqlClient::FetchTable(std::string const& table, std::vector<std::string>& outColumnNames, std::vector<std::vector<std::optional<std::string>>>& outRows)
{
    MYSQL* conn = AsMysql(_conn);

    outColumnNames.clear();
    outRows.clear();

    // Table names come from this tool's own compiled-in manifest (never user
    // input), so simple backtick-quoting is sufficient here.
    std::string query = "SELECT * FROM `" + table + "` ORDER BY `ID` ASC";

    if (mysql_query(conn, query.c_str()) != 0)
        return false;

    MYSQL_RES* res = mysql_store_result(conn);
    if (!res)
        return false;

    unsigned fieldCount = mysql_num_fields(res);
    MYSQL_FIELD* fields = mysql_fetch_fields(res);
    outColumnNames.reserve(fieldCount);
    for (unsigned i = 0; i < fieldCount; ++i)
        outColumnNames.emplace_back(fields[i].name);

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res)) != nullptr)
    {
        unsigned long* lengths = mysql_fetch_lengths(res);

        std::vector<std::optional<std::string>> outRow;
        outRow.reserve(fieldCount);
        for (unsigned i = 0; i < fieldCount; ++i)
        {
            if (row[i])
                outRow.emplace_back(std::string(row[i], lengths[i]));
            else
                outRow.emplace_back(std::nullopt);
        }
        outRows.push_back(std::move(outRow));
    }

    mysql_free_result(res);
    return true;
}

std::string MySqlClient::LastError() const
{
    return _conn ? mysql_error(AsMysql(_conn)) : "not connected";
}

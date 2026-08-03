#include "DbMerge.h"
#include <cstdlib>
#include <unordered_map>

namespace
{
    DbcValue ParseValue(char formatChar, std::optional<std::string> const& text, DbcValue const* existing)
    {
        if (!text && existing)
            return *existing;

        std::string const& s = text.value_or("");

        switch (FieldKindFor(formatChar))
        {
            case DbcFieldKind::Float:
                return s.empty() ? 0.0f : std::strtof(s.c_str(), nullptr);
            case DbcFieldKind::Byte:
                return static_cast<uint8_t>(s.empty() ? 0 : std::strtoul(s.c_str(), nullptr, 10));
            case DbcFieldKind::Str:
                return s;
            case DbcFieldKind::UInt:
            default:
                return static_cast<uint32_t>(s.empty() ? 0 : std::strtoul(s.c_str(), nullptr, 10));
        }
    }
}

bool ApplyDbRowsToTable(std::vector<std::string> const& columnNames,
    std::vector<std::vector<std::optional<std::string>>> const& dbRows,
    DbcTable& table, std::string* outError)
{
    if (columnNames.size() != table.format.size())
    {
        if (outError)
            *outError = "column count (" + std::to_string(columnNames.size()) +
                ") does not match DBC format length (" + std::to_string(table.format.size()) + ")";
        return false;
    }

    table.columnNames = columnNames;

    std::unordered_map<uint32_t, size_t> idToIndex;
    idToIndex.reserve(table.rows.size());
    for (size_t i = 0; i < table.rows.size(); ++i)
        idToIndex[table.IdOf(table.rows[i])] = i;

    for (auto const& dbRow : dbRows)
    {
        uint32_t id = std::get<uint32_t>(ParseValue(table.format[0], dbRow[0], nullptr));

        auto it = idToIndex.find(id);
        DbcRow const* baseline = it != idToIndex.end() ? &table.rows[it->second] : nullptr;

        DbcRow row;
        row.fields.reserve(table.format.size());
        for (size_t i = 0; i < table.format.size(); ++i)
            row.fields.push_back(ParseValue(table.format[i], dbRow[i], baseline ? &baseline->fields[i] : nullptr));

        if (it != idToIndex.end())
        {
            table.rows[it->second] = std::move(row);
        }
        else
        {
            idToIndex[id] = table.rows.size();
            table.rows.push_back(std::move(row));
        }
    }

    table.SortById();
    return true;
}

bool MergeDbRowsIntoTable(MySqlClient& db, std::string const& tableName, DbcTable& table, std::string* outError)
{
    std::vector<std::string> columnNames;
    std::vector<std::vector<std::optional<std::string>>> dbRows;

    if (!db.FetchTable(tableName, columnNames, dbRows))
    {
        if (outError)
            *outError = "query failed: " + db.LastError();
        return false;
    }

    table.tableName = tableName;
    return ApplyDbRowsToTable(columnNames, dbRows, table, outError);
}

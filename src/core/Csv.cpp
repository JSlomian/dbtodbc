#include "Csv.h"
#include <cstdio>
#include <sstream>

namespace
{
    std::string CsvEscape(std::string const& s)
    {
        bool needsQuoting = s.find_first_of(",\"\n\r") != std::string::npos;
        if (!needsQuoting)
            return s;

        std::string out = "\"";
        for (char c : s)
        {
            if (c == '"')
                out += "\"\"";
            else
                out += c;
        }
        out += "\"";
        return out;
    }

    std::string FormatFloat(float v)
    {
        char buf[64];
        std::snprintf(buf, sizeof(buf), "%.9g", v);
        return buf;
    }
}

std::string SerializeToCsv(DbcTable const& table)
{
    std::ostringstream out;

    for (size_t i = 0; i < table.columnNames.size(); ++i)
    {
        if (i)
            out << ',';
        out << CsvEscape(table.columnNames[i]);
    }
    out << "\r\n";

    for (DbcRow const& row : table.rows)
    {
        for (size_t i = 0; i < row.fields.size(); ++i)
        {
            if (i)
                out << ',';

            DbcValue const& value = row.fields[i];
            if (std::holds_alternative<uint32_t>(value))
                out << std::get<uint32_t>(value);
            else if (std::holds_alternative<float>(value))
                out << FormatFloat(std::get<float>(value));
            else if (std::holds_alternative<uint8_t>(value))
                out << static_cast<unsigned>(std::get<uint8_t>(value));
            else
                out << CsvEscape(std::get<std::string>(value));
        }
        out << "\r\n";
    }

    return out.str();
}

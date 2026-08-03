#pragma once

#include <algorithm>
#include <cstdint>
#include <string>
#include <variant>
#include <vector>

// A DBC/DB row field, stored generically regardless of its semantic meaning.
// Format-string chars map onto these alternatives by physical storage width:
//   'f'            -> Float   (4 bytes)
//   'b', 'X'       -> Byte    (1 byte)
//   's'            -> Str     (stored as text; written back as a pooled dbc string offset)
//   'i','n','d','x'-> UInt    (4 bytes, opaque for 'x')
using DbcValue = std::variant<uint32_t, float, uint8_t, std::string>;

enum class DbcFieldKind
{
    UInt,
    Float,
    Byte,
    Str,
};

inline DbcFieldKind FieldKindFor(char formatChar)
{
    switch (formatChar)
    {
        case 'f':
            return DbcFieldKind::Float;
        case 'b':
        case 'X':
            return DbcFieldKind::Byte;
        case 's':
            return DbcFieldKind::Str;
        default: // 'i', 'n', 'd', 'x'
            return DbcFieldKind::UInt;
    }
}

struct DbcRow
{
    std::vector<DbcValue> fields;
};

// A generic, format-driven representation of one *_dbc table's data - independent
// of AzerothCore's per-DBC typed structs (DBCStructure.h). Column 0 is always the
// `ID` column by AzerothCore's own *_dbc.sql convention (verified across all ~110
// tables), so it doubles as the merge/dedupe key without needing to special-case
// the format string's 'n'/'d' index-field marker.
struct DbcTable
{
    std::string dbcFileName;
    std::string tableName;
    std::string format;
    std::vector<std::string> columnNames; // from the DB schema, one per format char
    std::vector<DbcRow> rows;

    // Raw string-block bytes from the source DBC, preserved so the writer can seed
    // its output string block from them instead of building one from scratch - some
    // manifest formats mark fields 'x' (opaque) that are actually string-block
    // offsets in the real client layout (client-only text the server doesn't need),
    // and those stale offsets stay valid only if the bytes they point to persist.
    std::vector<uint8_t> originalStringBlock;

    [[nodiscard]] uint32_t IdOf(DbcRow const& row) const
    {
        return std::get<uint32_t>(row.fields[0]);
    }

    void SortById()
    {
        std::sort(rows.begin(), rows.end(), [this](DbcRow const& a, DbcRow const& b)
        {
            return IdOf(a) < IdOf(b);
        });
    }
};

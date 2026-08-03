#include "DbcWriter.h"
#include <cstdio>
#include <cstring>
#include <unordered_map>
#include <vector>

namespace
{
    uint32_t FieldWidth(char formatChar)
    {
        return (formatChar == 'b' || formatChar == 'X') ? 1u : 4u;
    }
}

bool WriteDbcFile(std::string const& path, DbcTable const& table)
{
    uint32_t fieldCount = static_cast<uint32_t>(table.format.size());
    uint32_t recordCount = static_cast<uint32_t>(table.rows.size());

    std::vector<uint32_t> fieldOffset(fieldCount, 0);
    for (uint32_t i = 1; i < fieldCount; ++i)
        fieldOffset[i] = fieldOffset[i - 1] + FieldWidth(table.format[i - 1]);

    uint32_t recordSize = fieldCount ? fieldOffset[fieldCount - 1] + FieldWidth(table.format[fieldCount - 1]) : 0;

    // String pool: seeded from the source file's original string block so any
    // field marked opaque ('x') in the format string that is actually a stale
    // string-block offset (client-only text the manifest doesn't track as a real
    // string) still points at valid bytes. Newly-interned strings are appended
    // after it. Offset 0 is always the empty string (WDBC convention).
    std::vector<char> stringBlock;
    if (!table.originalStringBlock.empty())
        stringBlock.assign(table.originalStringBlock.begin(), table.originalStringBlock.end());
    else
        stringBlock.push_back('\0');
    std::unordered_map<std::string, uint32_t> stringOffsets;
    stringOffsets.emplace("", 0u);

    // Index every string already present in the seeded block so re-writing an
    // already-loaded table reuses existing offsets instead of appending
    // duplicate copies on every write (which would make the writer non-idempotent).
    for (size_t pos = 0; pos < stringBlock.size(); )
    {
        size_t nul = pos;
        while (nul < stringBlock.size() && stringBlock[nul] != '\0')
            ++nul;
        stringOffsets.emplace(std::string(stringBlock.data() + pos, nul - pos), static_cast<uint32_t>(pos));
        pos = nul + 1;
    }

    auto internString = [&](std::string const& s) -> uint32_t
    {
        auto it = stringOffsets.find(s);
        if (it != stringOffsets.end())
            return it->second;

        uint32_t offset = static_cast<uint32_t>(stringBlock.size());
        stringBlock.insert(stringBlock.end(), s.begin(), s.end());
        stringBlock.push_back('\0');
        stringOffsets.emplace(s, offset);
        return offset;
    };

    std::vector<unsigned char> recordBlock(static_cast<size_t>(recordSize) * recordCount, 0);

    for (uint32_t r = 0; r < recordCount; ++r)
    {
        DbcRow const& row = table.rows[r];
        unsigned char* base = recordBlock.data() + static_cast<size_t>(r) * recordSize;

        for (uint32_t f = 0; f < fieldCount; ++f)
        {
            unsigned char* slot = base + fieldOffset[f];
            switch (FieldKindFor(table.format[f]))
            {
                case DbcFieldKind::Float:
                {
                    float v = std::get<float>(row.fields[f]);
                    std::memcpy(slot, &v, sizeof(v));
                    break;
                }
                case DbcFieldKind::Byte:
                {
                    uint8_t v = std::get<uint8_t>(row.fields[f]);
                    std::memcpy(slot, &v, sizeof(v));
                    break;
                }
                case DbcFieldKind::Str:
                {
                    uint32_t offset = internString(std::get<std::string>(row.fields[f]));
                    std::memcpy(slot, &offset, sizeof(offset));
                    break;
                }
                case DbcFieldKind::UInt:
                default:
                {
                    uint32_t v = std::get<uint32_t>(row.fields[f]);
                    std::memcpy(slot, &v, sizeof(v));
                    break;
                }
            }
        }
    }

    FILE* f = std::fopen(path.c_str(), "wb");
    if (!f)
        return false;

    uint32_t header[4] = { 0x43424457u /* 'WDBC' */, recordCount, fieldCount, recordSize };
    uint32_t stringSize = static_cast<uint32_t>(stringBlock.size());

    bool ok = true;
    ok = ok && std::fwrite(&header[0], sizeof(uint32_t), 1, f) == 1;      // magic
    ok = ok && std::fwrite(&header[1], sizeof(uint32_t), 1, f) == 1;      // recordCount
    ok = ok && std::fwrite(&header[2], sizeof(uint32_t), 1, f) == 1;      // fieldCount
    ok = ok && std::fwrite(&header[3], sizeof(uint32_t), 1, f) == 1;      // recordSize
    ok = ok && std::fwrite(&stringSize, sizeof(uint32_t), 1, f) == 1;     // stringSize
    if (ok && recordBlock.size())
        ok = std::fwrite(recordBlock.data(), 1, recordBlock.size(), f) == recordBlock.size();
    if (ok && stringBlock.size())
        ok = std::fwrite(stringBlock.data(), 1, stringBlock.size(), f) == stringBlock.size();

    std::fclose(f);
    return ok;
}

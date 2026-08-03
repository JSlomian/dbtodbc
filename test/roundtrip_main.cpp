// Verification harness (built as the `roundtrip_test` CMake target): constructs
// a raw WDBC file by hand, round-trips it through LoadDbcFile -> WriteDbcFile ->
// LoadDbcFile, and checks the two loaded tables are logically identical.
#include "../src/core/DbcReader.h"
#include "../src/core/DbcWriter.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>

namespace
{
    void WriteU32(std::vector<unsigned char>& buf, uint32_t v)
    {
        unsigned char bytes[4];
        std::memcpy(bytes, &v, 4);
        buf.insert(buf.end(), bytes, bytes + 4);
    }

    // format "niiss": ID(uint32), int(uint32), int(uint32), string, string
    std::vector<unsigned char> BuildRawDbc()
    {
        std::string const format = "niiss";
        uint32_t fieldCount = 5;
        uint32_t recordSize = 4 * 5; // all fields are 4-byte here

        // string block: offset 0 = "", then pool strings
        std::vector<char> strings;
        strings.push_back('\0');
        auto addStr = [&](std::string const& s) -> uint32_t
        {
            uint32_t off = static_cast<uint32_t>(strings.size());
            strings.insert(strings.end(), s.begin(), s.end());
            strings.push_back('\0');
            return off;
        };

        uint32_t offHello = addStr("Hello");
        uint32_t offWorld = addStr("World");
        uint32_t offEmpty = 0; // reuse the empty string at offset 0

        struct RowSpec { uint32_t id, a, b, sOff1, sOff2; };
        std::vector<RowSpec> rows = {
            { 5, 100, 200, offHello, offWorld },
            { 1, 111, 222, offEmpty, offHello },
            { 42, 999, 0, offWorld, offEmpty },
        };

        std::vector<unsigned char> file;
        WriteU32(file, 0x43424457u); // 'WDBC'
        WriteU32(file, static_cast<uint32_t>(rows.size()));
        WriteU32(file, fieldCount);
        WriteU32(file, recordSize);
        WriteU32(file, static_cast<uint32_t>(strings.size()));

        for (auto const& r : rows)
        {
            WriteU32(file, r.id);
            WriteU32(file, r.a);
            WriteU32(file, r.b);
            WriteU32(file, r.sOff1);
            WriteU32(file, r.sOff2);
        }

        file.insert(file.end(), strings.begin(), strings.end());
        return file;
    }

    bool TablesEqual(DbcTable const& a, DbcTable const& b)
    {
        if (a.rows.size() != b.rows.size())
        {
            std::cerr << "row count mismatch: " << a.rows.size() << " vs " << b.rows.size() << "\n";
            return false;
        }
        for (size_t r = 0; r < a.rows.size(); ++r)
        {
            if (a.rows[r].fields != b.rows[r].fields)
            {
                std::cerr << "row " << r << " mismatch\n";
                return false;
            }
        }
        return true;
    }
}

int main()
{
    std::string const format = "niiss";

    std::vector<unsigned char> raw = BuildRawDbc();
    FILE* f = std::fopen("rawtest.dbc", "wb");
    std::fwrite(raw.data(), 1, raw.size(), f);
    std::fclose(f);

    auto table1 = LoadDbcFile("rawtest.dbc", format);
    if (!table1)
    {
        std::cerr << "FAIL: could not load rawtest.dbc\n";
        return 1;
    }
    std::cout << "Loaded " << table1->rows.size() << " rows from hand-built WDBC file.\n";

    if (!WriteDbcFile("out1.dbc", *table1))
    {
        std::cerr << "FAIL: could not write out1.dbc\n";
        return 1;
    }

    auto table2 = LoadDbcFile("out1.dbc", format);
    if (!table2)
    {
        std::cerr << "FAIL: could not reload out1.dbc\n";
        return 1;
    }

    if (!TablesEqual(*table1, *table2))
    {
        std::cerr << "FAIL: round-trip tables differ\n";
        return 1;
    }

    // Round two: write out1's table again, and expect byte-identical output
    // (proves the writer is deterministic/idempotent once string interning
    // has stabilized).
    if (!WriteDbcFile("out2.dbc", *table2))
    {
        std::cerr << "FAIL: could not write out2.dbc\n";
        return 1;
    }

    std::ifstream a("out1.dbc", std::ios::binary);
    std::ifstream b("out2.dbc", std::ios::binary);
    std::string ca((std::istreambuf_iterator<char>(a)), std::istreambuf_iterator<char>());
    std::string cb((std::istreambuf_iterator<char>(b)), std::istreambuf_iterator<char>());
    if (ca != cb)
    {
        std::cerr << "FAIL: out1.dbc and out2.dbc differ (writer not idempotent)\n";
        return 1;
    }

    std::cout << "PASS: round-trip load -> write -> load -> write is logically and "
                 "byte-for-byte stable.\n";
    return 0;
}

#include "core/Config.h"
#include "core/Csv.h"
#include "core/DbMerge.h"
#include "core/DbcManifest.h"
#include "core/DbcReader.h"
#include "core/DbcWriter.h"
#include "core/MySqlClient.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <unordered_set>

namespace fs = std::filesystem;

namespace
{
    // These gt*.dbc manifest entries declare format "df" (2 fields) but the real
    // WotLK 3.3.5a client files are single-float tables (fieldCount=1, recordSize=4),
    // so LoadDbcFile always rejects them. Filtered out here until the format
    // strings themselves get fixed. gtOCTClassCombatRatingScalar.dbc is genuinely
    // 2-field and is NOT in this list.
    std::unordered_set<std::string> const KnownBrokenDbcFiles = {
        "gtBarberShopCostBase.dbc",
        "gtCombatRatings.dbc",
        "gtChanceToMeleeCritBase.dbc",
        "gtChanceToMeleeCrit.dbc",
        "gtChanceToSpellCritBase.dbc",
        "gtChanceToSpellCrit.dbc",
        "gtNPCManaCostScaler.dbc",
        "gtOCTRegenHP.dbc",
        "gtRegenHPPerSpt.dbc",
        "gtRegenMPPerSpt.dbc",
    };

    std::string StemOf(std::string const& dbcFileName)
    {
        fs::path p(dbcFileName);
        return p.stem().string();
    }

    bool WriteStringToFile(fs::path const& path, std::string const& content)
    {
        std::ofstream file(path, std::ios::binary | std::ios::trunc);
        if (!file.is_open())
            return false;
        file << content;
        return file.good();
    }

    bool ReadStringFromFile(fs::path const& path, std::string& out)
    {
        std::ifstream file(path, std::ios::binary);
        if (!file.is_open())
            return false;
        out.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
        return true;
    }
}

int main(int argc, char** argv)
{
    auto pauseBeforeExit = []()
    {
        std::cout << "\nPress Enter to exit...";
        std::cin.get();
    };

    std::string configPath = argc > 1 ? argv[1] : "dbtodbc.conf";

    std::string err;
    std::optional<Config> configOpt = Config::LoadFromFile(configPath, &err);
    if (!configOpt)
    {
        std::cerr << "Failed to load config '" << configPath << "': " << err << "\n";
        pauseBeforeExit();
        return 1;
    }
    Config const& cfg = *configOpt;

    fs::create_directories(cfg.outputDbcDir);
    fs::create_directories(cfg.outputCsvDir);

    MySqlClient db;
    if (!db.Connect(cfg.dbHost, cfg.dbPort, cfg.dbUser, cfg.dbPassword, cfg.dbName))
    {
        std::cerr << "Failed to connect to MySQL at " << cfg.dbHost << ":" << cfg.dbPort
                   << " db=" << cfg.dbName << ": " << db.LastError() << "\n";
        pauseBeforeExit();
        return 1;
    }

    int processed = 0, unchanged = 0, changed = 0, skippedMissingFile = 0, skippedEmptyDbTable = 0, skippedKnownBroken = 0, errors = 0;
    std::vector<std::string> generatedFiles;

    for (ManifestEntry const& entry : GetDbcManifest())
    {
        if (KnownBrokenDbcFiles.count(entry.dbcFile))
        {
            ++skippedKnownBroken;
            continue;
        }

        fs::path dbcPath = fs::path(cfg.dbcSourceDir) / entry.dbcFile;
        if (!fs::exists(dbcPath))
        {
            ++skippedMissingFile;
            continue;
        }

        std::optional<DbcTable> tableOpt = LoadDbcFile(dbcPath.string(), entry.format);
        if (!tableOpt)
        {
            std::cerr << "[error] failed to load " << entry.dbcFile << "\n";
            ++errors;
            continue;
        }

        DbcTable table = std::move(*tableOpt);
        table.dbcFileName = entry.dbcFile;
        table.tableName = entry.table;

        fs::path csvSnapshotPath = fs::path(cfg.outputCsvDir) / (StemOf(entry.dbcFile) + ".csv");

        std::string csvText;

        if (db.TableExists(entry.table))
        {
            std::vector<std::string> columnNames;
            std::vector<std::vector<std::optional<std::string>>> dbRows;
            if (!db.FetchTable(entry.table, columnNames, dbRows))
            {
                std::cerr << "[error] " << entry.table << ": query failed: " << db.LastError() << "\n";
                ++errors;
                continue;
            }

            if (dbRows.empty())
            {
                ++skippedEmptyDbTable;
                continue;
            }

            if (columnNames.size() != table.format.size())
            {
                std::cerr << "[error] " << entry.table << ": column count (" << columnNames.size()
                           << ") does not match DBC format length (" << table.format.size() << ")\n";
                ++errors;
                continue;
            }

            table.columnNames = columnNames;

            std::string mergeErr;
            if (!ApplyDbRowsToTable(columnNames, dbRows, table, &mergeErr))
            {
                std::cerr << "[error] " << entry.table << ": " << mergeErr << "\n";
                ++errors;
                continue;
            }

            csvText = SerializeToCsv(table);

            ++processed;

            std::string baselineCsv;
            if (ReadStringFromFile(csvSnapshotPath, baselineCsv) && baselineCsv == csvText)
            {
                ++unchanged;
                continue;
            }
        }
        else
        {
            table.columnNames.reserve(table.format.size());
            for (size_t i = 0; i < table.format.size(); ++i)
                table.columnNames.push_back("field" + std::to_string(i));
            table.SortById();

            ++processed;
            csvText = SerializeToCsv(table);
        }

        fs::path outputDbcPath = fs::path(cfg.outputDbcDir) / entry.dbcFile;
        if (!WriteDbcFile(outputDbcPath.string(), table))
        {
            std::cerr << "[error] failed to write " << outputDbcPath << "\n";
            ++errors;
            continue;
        }

        if (!WriteStringToFile(csvSnapshotPath, csvText))
        {
            std::cerr << "[error] failed to write " << csvSnapshotPath << "\n";
            ++errors;
            continue;
        }

        ++changed;
        generatedFiles.push_back(outputDbcPath.string());
        std::cout << "[changed] " << entry.dbcFile << " (" << entry.table << ")\n";
    }

    std::cout << "\nGenerated DBC files (" << generatedFiles.size() << "):\n";
    for (std::string const& f : generatedFiles)
        std::cout << "  " << f << "\n";

    std::cout << "\nProcessed " << processed << " DBC files: "
               << unchanged << " unchanged, " << changed << " changed, "
               << skippedMissingFile << " skipped (missing .dbc), "
               << skippedEmptyDbTable << " skipped (empty DB table), "
               << skippedKnownBroken << " skipped (known-broken format), "
               << errors << " errors.\n";

    pauseBeforeExit();
    return errors > 0 ? 1 : 0;
}

#pragma once

#include <vector>

struct ManifestEntry
{
    char const* dbcFile;   // e.g. "Item.dbc"
    char const* format;    // DBCfmt.h format string, e.g. "niiiiiii"
    char const* table;     // e.g. "item_dbc"
};

// One-time transcription of every LOAD_DBC(...) call + matching DBCStorage<T> format
// constant in AzerothCore's src/server/game/DataStores/DBCStores.cpp, joined with
// the format strings from src/server/shared/DataStores/DBCfmt.h. Regenerate by
// re-running the extraction if azerothcore-wotlk's DBC set changes.
std::vector<ManifestEntry> const& GetDbcManifest();

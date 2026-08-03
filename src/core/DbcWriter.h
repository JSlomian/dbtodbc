#pragma once

#include "DbcValue.h"
#include <string>

// Inverse of DbcReader/DBCFileLoader: serializes a DbcTable back into a binary
// WDBC file (header + fixed-width records + deduplicated string block) that
// AzerothCore's own DBCFileLoader can read back unchanged.
bool WriteDbcFile(std::string const& path, DbcTable const& table);

#pragma once

#include "DbcValue.h"
#include <optional>
#include <string>

// Loads a raw .dbc file into a generic DbcTable using the vendored DBCFileLoader,
// reading every field (including 'x'/'X' unused slots and 'd' sort fields) directly
// via DBCFileLoader::Record accessors rather than AutoProduceData's reduced struct
// layout - so no data is dropped and the table can be losslessly written back out.
std::optional<DbcTable> LoadDbcFile(std::string const& path, std::string const& format);

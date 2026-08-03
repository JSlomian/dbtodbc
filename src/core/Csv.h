#pragma once

#include "DbcValue.h"
#include <string>

// Serializes a DbcTable to CSV text (one column per format field, headers taken
// from the DB schema). Used purely for human review and change-detection - the
// tool never parses CSV back in, it only compares freshly generated text against
// the previous run's snapshot file.
std::string SerializeToCsv(DbcTable const& table);

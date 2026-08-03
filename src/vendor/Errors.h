// Minimal stand-in for AzerothCore src/common/Debugging/Errors.h.
// The vendored DBCFileLoader only ever uses the single-argument ASSERT(cond)
// form (including "cond && \"message\"" style), so this drops the
// Acore::StringFormat/fmt-based variadic machinery entirely.
#ifndef DBTODBC_ERRORS_H
#define DBTODBC_ERRORS_H

#include <cstdio>
#include <cstdlib>

#define ASSERT(cond) \
    do { \
        if (!(cond)) \
        { \
            std::fprintf(stderr, "Assertion failed: %s\n  at %s:%d\n", #cond, __FILE__, __LINE__); \
            std::abort(); \
        } \
    } while (0)

#endif

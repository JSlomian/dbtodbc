// Trimmed from AzerothCore src/common/Define.h - keeps only the typedefs and
// endianness macro that DBCFileLoader/ByteConverter depend on.
#ifndef DBTODBC_DEFINE_H
#define DBTODBC_DEFINE_H

#include "CompilerDefs.h"
#include <cinttypes>
#include <climits>

#define ACORE_LITTLEENDIAN 0
#define ACORE_BIGENDIAN    1

#if !defined(ACORE_ENDIAN)
#  if defined (BOOST_BIG_ENDIAN)
#    define ACORE_ENDIAN ACORE_BIGENDIAN
#  else
#    define ACORE_ENDIAN ACORE_LITTLEENDIAN
#  endif
#endif

typedef std::int64_t int64;
typedef std::int32_t int32;
typedef std::int16_t int16;
typedef std::int8_t int8;
typedef std::uint64_t uint64;
typedef std::uint32_t uint32;
typedef std::uint16_t uint16;
typedef std::uint8_t uint8;

#endif //DBTODBC_DEFINE_H

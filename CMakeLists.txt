cmake_minimum_required(VERSION 3.20)
project(dbtodbc CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

# NOTE: this project's default GNU `ld` (binutils 2.43.1, MSYS2 UCRT64) reliably
# crashes (exit 116, no diagnostic) linking this project's full object set -
# reproduces regardless of optimization/debug-info level, so it's a linker
# bug/environment issue, not something in this code. `lld` links the identical
# objects successfully, so prefer it when available.
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    include(CheckCXXCompilerFlag)
    set(CMAKE_REQUIRED_FLAGS "-fuse-ld=lld")
    check_cxx_compiler_flag("-fuse-ld=lld" HAVE_LLD)
    unset(CMAKE_REQUIRED_FLAGS)
    if(HAVE_LLD)
        add_link_options(-fuse-ld=lld)
    endif()
endif()

find_path(MARIADB_INCLUDE_DIR
    NAMES mysql.h
    PATH_SUFFIXES mariadb mysql
)
find_library(MARIADB_LIBRARY
    NAMES mariadbclient libmariadb mariadb libmysql mysqlclient
)

if(NOT MARIADB_INCLUDE_DIR OR NOT MARIADB_LIBRARY)
    message(FATAL_ERROR
        "MariaDB/MySQL client library not found. "
        "On MSYS2/UCRT64: pacman -S mingw-w64-ucrt-x86_64-libmariadbclient  "
        "On Debian/Ubuntu: apt install libmariadb-dev  "
        "Then re-run cmake."
    )
endif()

message(STATUS "MariaDB include dir: ${MARIADB_INCLUDE_DIR}")
message(STATUS "MariaDB library: ${MARIADB_LIBRARY}")

add_executable(dbtodbc
    src/main.cpp
    src/vendor/DBCFileLoader.cpp
    src/core/Config.cpp
    src/core/Csv.cpp
    src/core/DbMerge.cpp
    src/core/DbcManifest.cpp
    src/core/DbcReader.cpp
    src/core/DbcWriter.cpp
    src/core/MySqlClient.cpp
)

target_include_directories(dbtodbc PRIVATE
    src
    src/vendor
    src/core
    ${MARIADB_INCLUDE_DIR}
)

target_link_libraries(dbtodbc PRIVATE ${MARIADB_LIBRARY})

if(WIN32)
    target_link_libraries(dbtodbc PRIVATE ws2_32 secur32 bcrypt)
endif()

add_executable(roundtrip_test
    test/roundtrip_main.cpp
    src/vendor/DBCFileLoader.cpp
    src/core/DbcReader.cpp
    src/core/DbcWriter.cpp
)

target_include_directories(roundtrip_test PRIVATE
    src
    src/vendor
    src/core
)

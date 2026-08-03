#pragma once

#include <optional>
#include <string>

// Loads a simple KEY=VALUE .env/.conf file (# comments, blank lines ignored,
// optional surrounding quotes on values).
struct Config
{
    std::string dbHost = "127.0.0.1";
    unsigned dbPort = 3306;
    std::string dbUser;
    std::string dbPassword;
    std::string dbName;
    std::string dbcSourceDir;
    std::string outputDbcDir;
    std::string outputCsvDir;

    static std::optional<Config> LoadFromFile(std::string const& path, std::string* outError);
};

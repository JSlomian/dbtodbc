#include "Config.h"
#include <fstream>
#include <sstream>
#include <unordered_map>

namespace
{
    std::string Trim(std::string s)
    {
        size_t begin = s.find_first_not_of(" \t\r\n");
        if (begin == std::string::npos)
            return "";
        size_t end = s.find_last_not_of(" \t\r\n");
        return s.substr(begin, end - begin + 1);
    }

    std::string StripQuotes(std::string s)
    {
        if (s.size() >= 2 && ((s.front() == '"' && s.back() == '"') || (s.front() == '\'' && s.back() == '\'')))
            return s.substr(1, s.size() - 2);
        return s;
    }
}

std::optional<Config> Config::LoadFromFile(std::string const& path, std::string* outError)
{
    std::ifstream file(path);
    if (!file.is_open())
    {
        if (outError)
            *outError = "could not open config file: " + path;
        return std::nullopt;
    }

    std::unordered_map<std::string, std::string> values;
    std::string line;
    while (std::getline(file, line))
    {
        std::string trimmed = Trim(line);
        if (trimmed.empty() || trimmed[0] == '#' || trimmed[0] == ';')
            continue;

        size_t eq = trimmed.find('=');
        if (eq == std::string::npos)
            continue;

        std::string key = Trim(trimmed.substr(0, eq));
        std::string value = StripQuotes(Trim(trimmed.substr(eq + 1)));
        values[key] = value;
    }

    auto require = [&](char const* key, std::string& err) -> std::optional<std::string>
    {
        auto it = values.find(key);
        if (it == values.end() || it->second.empty())
        {
            err = std::string("missing required config key: ") + key;
            return std::nullopt;
        }
        return it->second;
    };

    Config cfg;
    std::string err;

    if (auto v = require("DB_USER", err))
        cfg.dbUser = *v;
    else { if (outError) *outError = err; return std::nullopt; }

    if (auto v = require("DB_NAME", err))
        cfg.dbName = *v;
    else { if (outError) *outError = err; return std::nullopt; }

    if (auto v = require("DBC_SOURCE_DIR", err))
        cfg.dbcSourceDir = *v;
    else { if (outError) *outError = err; return std::nullopt; }

    if (auto v = require("OUTPUT_DBC_DIR", err))
        cfg.outputDbcDir = *v;
    else { if (outError) *outError = err; return std::nullopt; }

    if (auto v = require("OUTPUT_CSV_DIR", err))
        cfg.outputCsvDir = *v;
    else { if (outError) *outError = err; return std::nullopt; }

    if (auto it = values.find("DB_PASSWORD"); it != values.end())
        cfg.dbPassword = it->second;

    if (auto it = values.find("DB_HOST"); it != values.end() && !it->second.empty())
        cfg.dbHost = it->second;

    if (auto it = values.find("DB_PORT"); it != values.end() && !it->second.empty())
        cfg.dbPort = static_cast<unsigned>(std::stoul(it->second));

    return cfg;
}

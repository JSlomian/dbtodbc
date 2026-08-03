#include "DbcReader.h"
#include "DBCFileLoader.h"

std::optional<DbcTable> LoadDbcFile(std::string const& path, std::string const& format)
{
    DBCFileLoader loader;
    if (!loader.Load(path.c_str(), format.c_str()))
        return std::nullopt;

    if (loader.GetCols() != format.size())
        return std::nullopt;

    DbcTable table;
    table.format = format;
    table.originalStringBlock.assign(loader.GetStringBlock(), loader.GetStringBlock() + loader.GetStringSize());

    uint32_t fieldCount = loader.GetCols();
    uint32_t rowCount = loader.GetNumRows();
    table.rows.reserve(rowCount);

    for (uint32_t r = 0; r < rowCount; ++r)
    {
        DBCFileLoader::Record record = loader.getRecord(r);

        DbcRow row;
        row.fields.reserve(fieldCount);

        for (uint32_t f = 0; f < fieldCount; ++f)
        {
            switch (FieldKindFor(format[f]))
            {
                case DbcFieldKind::Float:
                    row.fields.emplace_back(record.getFloat(f));
                    break;
                case DbcFieldKind::Byte:
                    row.fields.emplace_back(record.getUInt8(f));
                    break;
                case DbcFieldKind::Str:
                    row.fields.emplace_back(std::string(record.getString(f)));
                    break;
                case DbcFieldKind::UInt:
                default:
                    row.fields.emplace_back(record.getUInt(f));
                    break;
            }
        }

        table.rows.push_back(std::move(row));
    }

    return table;
}

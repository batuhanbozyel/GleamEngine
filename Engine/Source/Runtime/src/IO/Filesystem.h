#pragma once
#include <filesystem>

namespace Gleam {

namespace Filesystem = std::filesystem;

} // namespace Gleam

namespace std {

template <>
struct hash<filesystem::path>
{
    std::size_t operator()(const filesystem::path& path) const
    {
        return hash_value(path);
    }
};

} // namespace std

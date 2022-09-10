#pragma once
#include <filesystem>

namespace Gleam {

namespace Filesystem = std::filesystem;

} // namespace Gleam

#ifndef _MSC_VER
namespace std {

template <>
struct hash<filesystem::path>
{
    size_t operator()(const filesystem::path& path) const
    {
        return hash_value(path);
    }
};

} // namespace std
#endif
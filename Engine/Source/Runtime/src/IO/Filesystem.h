#pragma once
#include <filesystem>

namespace Gleam {

class File;
enum class FileType;

class Filesystem
{
public:
	using Path = std::filesystem::path;
	using DirectoryIterator = std::filesystem::directory_iterator;

	static File Create(const Filesystem::Path& path, FileType type);

	static File Open(const Filesystem::Path& path, FileType type);

	static Path WorkingDirectory();

	static Path Relative(const Path& path, const Path& base);

	static bool Exists(const Path& path);

	static bool IsDirectory(const Path& path);

};

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

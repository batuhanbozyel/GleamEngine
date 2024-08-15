#include "gpch.h"
#include "Filesystem.h"

using namespace Gleam;

Filesystem::Path Filesystem::WorkingDirectory()
{
	return std::filesystem::current_path();
}

Filesystem::Path Filesystem::Relative(const Filesystem::Path& path, const Filesystem::Path& base)
{
	return std::filesystem::relative(path, base);
}

bool Filesystem::Exists(const Filesystem::Path& path)
{
	return std::filesystem::exists(path);
}

bool Filesystem::IsDirectory(const Filesystem::Path& path)
{
	return std::filesystem::is_directory(path);
}
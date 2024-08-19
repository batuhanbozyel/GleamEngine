#include "gpch.h"
#include "Filesystem.h"
#include "File.h"

using namespace Gleam;

File Filesystem::Create(const Filesystem::Path& path, FileType type)
{
    auto flags = std::ios::out | std::ios::in | std::ios::app;
    if (type == FileType::Binary)
    {
        flags |= std::ios::binary;
    }
    FileStream handle(path, flags);
    handle.unsetf(std::ios::skipws);
    
    std::lock_guard<std::mutex> lock(mFileCreateMutex);
    if (auto it = mFileAccessors.find(path); it != mFileAccessors.end())
    {
        return File(std::move(handle), path, it->second);
    }
    
    auto it = mFileAccessors.emplace_hint(mFileAccessors.end(),
                                          std::piecewise_construct,
                                          std::forward_as_tuple(path),
                                          std::forward_as_tuple());
	return File(std::move(handle), path, it->second);
}

File Filesystem::Open(const Filesystem::Path& path, FileType type)
{
	auto flags = std::ios::out | std::ios::in;
	if (type == FileType::Binary)
	{
		flags |= std::ios::binary;
	}
	FileStream handle(path, flags);
	handle.unsetf(std::ios::skipws);
    
    std::lock_guard<std::mutex> lock(mFileCreateMutex);
	return File(std::move(handle), path, mFileAccessors[path]);
}

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

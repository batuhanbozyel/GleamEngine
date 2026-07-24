#include "gpch.h"
#include "Filesystem.h"
#include "File.h"

using namespace Gleam;

void Filesystem::ForEach(const Path& path, const DirectoryFn& fn, bool recursive)
{
	if (path.Empty())
	{
		return;
	}

	std::filesystem::path stlPath = std::wstring_view(path.Native().c_str(), path.Native().length());

	std::error_code error;
    for (const auto& node : std::filesystem::directory_iterator(stlPath, error))
    {
		auto entry = DirectoryEntry(Path(node), node.is_directory(error));
        if (recursive && entry.IsDirectory())
        {
            ForEach(entry, fn, recursive);
        }
        else
        {
            fn(entry);
        }
    }
}

WriteAccessor<File> Filesystem::Create(const Path& path, FileType type)
{
    auto flags = std::ios::out | std::ios::in | std::ios::trunc;
    if (type == FileType::Binary)
    {
        flags |= std::ios::binary;
    }
	std::filesystem::path stlPath = std::wstring_view(path.Native().c_str(), path.Native().length());
    FileStream handle(stlPath, flags);
    handle.unsetf(std::ios::skipws);
    
    std::lock_guard<std::mutex> lock(mFileCreateMutex);
	auto it = mFileAccessors.find(path);
    if (it == mFileAccessors.end())
    {
		it = mFileAccessors.emplace_hint(mFileAccessors.end(),
										 eastl::piecewise_construct,
										 eastl::forward_as_tuple(path),
										 eastl::forward_as_tuple());
    }
	return WriteAccessor<File>(File(std::move(handle), path, it->second), it->second);
}

ReadAccessor<File> Filesystem::OpenRead(const Path& path, FileType type)
{
	auto flags = std::ios::out | std::ios::in;
	if (type == FileType::Binary)
	{
		flags |= std::ios::binary;
	}
	std::filesystem::path stlPath = std::wstring_view(path.Native().c_str(), path.Native().length());
	FileStream handle(stlPath, flags);
	handle.unsetf(std::ios::skipws);
    
    std::lock_guard<std::mutex> lock(mFileCreateMutex);
	auto it = mFileAccessors.find(path);
    if (it == mFileAccessors.end())
    {
		it = mFileAccessors.emplace_hint(mFileAccessors.end(),
										 eastl::piecewise_construct,
										 eastl::forward_as_tuple(path),
										 eastl::forward_as_tuple());
    }
	return ReadAccessor<File>(File(std::move(handle), path, it->second), it->second);
}

WriteAccessor<File> Filesystem::OpenWrite(const Path& path, FileType type)
{
    auto flags = std::ios::out | std::ios::in;
    if (type == FileType::Binary)
    {
        flags |= std::ios::binary;
    }
    std::filesystem::path stlPath = std::wstring_view(path.Native().c_str(), path.Native().length());
    FileStream handle(stlPath, flags);
    handle.unsetf(std::ios::skipws);
    
    std::lock_guard<std::mutex> lock(mFileCreateMutex);
	auto it = mFileAccessors.find(path);
    if (it == mFileAccessors.end())
    {
        it = mFileAccessors.emplace_hint(mFileAccessors.end(),
                                          eastl::piecewise_construct,
                                          eastl::forward_as_tuple(path),
                                          eastl::forward_as_tuple());
    }
    return WriteAccessor<File>(File(std::move(handle), path, it->second), it->second);
}

bool Filesystem::Remove(const Path& path)
{
	std::filesystem::path stlPath = std::wstring_view(path.Native().c_str(), path.Native().length());
    return std::filesystem::remove(stlPath);
}

FileAccessor& Filesystem::Accessor(const Path& path)
{
	return mFileAccessors[path];
}

Path Filesystem::WorkingDirectory()
{
	return std::filesystem::current_path();
}

Path Filesystem::Relative(const Path& path, const Path& base)
{
	std::filesystem::path stlPath = std::wstring_view(path.Native().c_str(), path.Native().length());
	std::filesystem::path stlBase = std::wstring_view(base.Native().c_str(), base.Native().length());
	return Path(std::filesystem::relative(stlPath, stlBase));
}

bool Filesystem::Exists(const Path& path)
{
	if (path.Empty())
	{
		return false;
	}

#ifdef PLATFORM_WINDOWS
	DWORD attrs = ::GetFileAttributesW(path.Native().c_str());
	return attrs != INVALID_FILE_ATTRIBUTES;
#else
	TString utf8Path;
	utf8Path.append_convert(path.Native());
	struct stat statBuf;
	return stat(utf8Path.c_str(), &statBuf) == 0;
#endif
}

bool Filesystem::IsDirectory(const Path& path)
{
	if (path.Empty())
	{
		return false;
	}

#ifdef PLATFORM_WINDOWS
	DWORD attrs = ::GetFileAttributesW(path.Native().c_str());
	return (attrs != INVALID_FILE_ATTRIBUTES) && (attrs & FILE_ATTRIBUTE_DIRECTORY);
#else
	TString utf8Path;
	utf8Path.append_convert(path.Native());
	struct stat statBuf;
	return (stat(utf8Path.c_str(), &statBuf) == 0) && S_ISDIR(statBuf.st_mode);
#endif
}

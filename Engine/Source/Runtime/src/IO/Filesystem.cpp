#include "gpch.h"
#include "Filesystem.h"
#include "File.h"
#include "Log.h"

#include <cerrno>
#include <cstring>

using namespace Gleam;

void Filesystem::ForEach(const Path& path, const DirectoryFn& fn, bool recursive)
{
	if (path.Empty())
	{
		return;
	}

	std::filesystem::path stlPath = std::wstring_view(path.Native().c_str(), path.Native().length());

	std::error_code error;
	auto directory = std::filesystem::directory_iterator(stlPath, error);
	if (error)
	{
		GLEAM_CORE_ERROR("Filesystem failed to iterate directory: {0} ({1})", path.String(), error.message());
		return;
	}

    for (const auto& node : directory)
    {
		std::error_code entryError;
		auto entry = DirectoryEntry(Path(node), node.is_directory(entryError));
		if (entryError)
		{
			GLEAM_CORE_ERROR("Filesystem::ForEach failed to query directory entry: {0} ({1})", entry.String(), entryError.message());
		}
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
    if (not handle.is_open())
    {
        const char* reason = std::strerror(errno);
        GLEAM_CORE_ERROR("Filesystem failed to create file: {0} ({1})", path.String(), reason);
    }
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
	auto flags = std::ios::in;
	if (type == FileType::Binary)
	{
		flags |= std::ios::binary;
	}
	std::filesystem::path stlPath = std::wstring_view(path.Native().c_str(), path.Native().length());
	FileStream handle(stlPath, flags);
	if (not handle.is_open())
	{
		const char* reason = std::strerror(errno);
		GLEAM_CORE_ERROR("Filesystem failed to open file for read: {0} ({1})", path.String(), reason);
	}
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
    if (not handle.is_open())
    {
        const char* reason = std::strerror(errno);
        GLEAM_CORE_ERROR("Filesystem failed to open file for write: {0} ({1})", path.String(), reason);
    }
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

bool Filesystem::CreateDirectories(const Path& path)
{
	std::filesystem::path stlPath = std::wstring_view(path.Native().c_str(), path.Native().length());

	std::error_code error;
	if (not std::filesystem::create_directories(stlPath, error))
	{
		GLEAM_CORE_ERROR("Filesystem failed to create directory: {0} ({1})", path.String(), error.message());
		return false;
	}
	return true;
}

bool Filesystem::Remove(const Path& path)
{
	std::filesystem::path stlPath = std::wstring_view(path.Native().c_str(), path.Native().length());

	std::error_code error;
	if (not std::filesystem::remove(stlPath, error))
	{
		GLEAM_CORE_ERROR("Filesystem failed to remove file: {0} ({1})", path.String(), error.message());
		return false;
	}
	return true;
}

FileAccessor& Filesystem::Accessor(const Path& path)
{
	return mFileAccessors[path];
}

Path Filesystem::WorkingDirectory()
{
	return std::filesystem::current_path();
}

Path Filesystem::AppDataDirectory()
{
#if defined(PLATFORM_WINDOWS)
	wchar_t* appData = nullptr;
	size_t length = 0;
	Path path;
	if (_wdupenv_s(&appData, &length, L"APPDATA") == 0 and appData)
	{
		path = Path(appData);
		free(appData);
	}
	return path;
#elif defined(PLATFORM_MACOS)
	const char* home = std::getenv("HOME");
	return home ? Path(home) / "Library" / "Application Support" : Path();
#endif
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

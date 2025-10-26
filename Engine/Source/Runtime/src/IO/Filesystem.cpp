#include "gpch.h"
#include "Filesystem.h"
#include "File.h"

using namespace Gleam;

void Filesystem::ForEach(const Path& path, const DirectoryFn& fn, bool recursive)
{
	std::filesystem::path stlPath = std::wstring_view(path.Native().c_str(), path.Native().length());
    for (auto& node : std::filesystem::directory_iterator(stlPath))
    {
		Path nodePath = Path(node);
        if (recursive && IsDirectory(nodePath))
        {
            ForEach(nodePath, fn, recursive);
        }
        else
        {
            fn(nodePath);
        }
    }
}

File Filesystem::Create(const Path& path, FileType type)
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
    if (auto it = mFileAccessors.find(path); it != mFileAccessors.end())
    {
        return File(std::move(handle), path, it->second);
    }
    
    auto it = mFileAccessors.emplace_hint(mFileAccessors.end(),
                                          eastl::piecewise_construct,
                                          eastl::forward_as_tuple(path),
                                          eastl::forward_as_tuple());
	return File(std::move(handle), path, it->second);
}

File Filesystem::Open(const Path& path, FileType type)
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
	return File(eastl::move(handle), path, mFileAccessors[path]);
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

FileAccessor::Read Filesystem::ReadAccessor(const Path& path)
{
	return FileAccessor::Read(Accessor(path));
}

FileAccessor::Write Filesystem::WriteAccessor(const Path& path)
{
	return FileAccessor::Write(Accessor(path));
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

// File::Accessors

FileAccessor::Write::Write(FileAccessor& accessor)
	: mAccessor(accessor)
{
	std::unique_lock<std::mutex> lock(mAccessor.mutex);
	mAccessor.condition.wait(lock, [this]
	{
		return mAccessor.status == FileStatus::Available;
	});
	mAccessor.status = FileStatus::Writing;
}

FileAccessor::Write::~Write()
{
	mAccessor.status = FileStatus::Available;
	mAccessor.condition.notify_all();
}

FileAccessor::Read::Read(FileAccessor& accessor)
	: mAccessor(accessor)
	, mLock(accessor.mutex)
{
	mAccessor.condition.wait(mLock, [this]
	{
		return mAccessor.status != FileStatus::Writing;
	});

	++mAccessor.concurrentReaders;
	mAccessor.status = FileStatus::Reading;
	mLock.unlock();
}

FileAccessor::Read::~Read()
{
	mLock.lock();
	if (--mAccessor.concurrentReaders == 0)
	{
		mAccessor.status = FileStatus::Available;
		mAccessor.condition.notify_all();
	}
}

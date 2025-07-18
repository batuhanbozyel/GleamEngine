#include "gpch.h"
#include "Filesystem.h"
#include "File.h"

using namespace Gleam;

void Filesystem::ForEach(const Path& path, const DirectoryFn& fn, bool recursive)
{
    for (auto& node : std::filesystem::directory_iterator(path))
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
    FileStream handle(path, flags);
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
	FileStream handle(path, flags);
	handle.unsetf(std::ios::skipws);
    
    std::lock_guard<std::mutex> lock(mFileCreateMutex);
	return File(eastl::move(handle), path, mFileAccessors[path]);
}

bool Filesystem::Remove(const Path& path)
{
    return std::filesystem::remove(path);
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
	return std::filesystem::relative(path, base);
}

bool Filesystem::Exists(const Path& path)
{
	return std::filesystem::exists(path);
}

bool Filesystem::IsDirectory(const Path& path)
{
	return std::filesystem::is_directory(path);
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

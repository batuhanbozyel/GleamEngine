#pragma once
#include <Reflection/Macro.h>
#include "Container/Hash.h"
#include "Path.h"

#include <mutex>
#include <fstream>
#include <functional>
#include <filesystem>

namespace Gleam {

class File;
enum class FileType;
using FileStream = std::fstream;

enum class FileStatus
{
    Available,
    Writing,
    Reading
};

struct FileAccessor
{
	std::mutex mutex;
	std::condition_variable condition;
	std::atomic<uint32_t> concurrentReaders = 0;
	FileStatus status = FileStatus::Available;
};

template<typename T>
class ReadAccessor
{
public:
	ReadAccessor(T&& resource, FileAccessor& accessor)
		: mResource(std::move(resource))
		, mAccessor(accessor)
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

	~ReadAccessor()
	{
		mLock.lock();
		if (--mAccessor.concurrentReaders == 0)
		{
			mAccessor.status = FileStatus::Available;
			mAccessor.condition.notify_all();
		}
	}

	ReadAccessor(const ReadAccessor&) = delete;
	ReadAccessor& operator=(const ReadAccessor&) = delete;

	ReadAccessor(ReadAccessor&&) = default;
	ReadAccessor& operator=(ReadAccessor&&) = default;

	const T* operator->() const { return &mResource; }
	const T& operator*() const { return mResource; }
	const T& Get() const { return mResource; }

private:
	T mResource;
	FileAccessor& mAccessor;
	std::unique_lock<std::mutex> mLock;
};

template<typename T>
class WriteAccessor
{
public:
	WriteAccessor(T&& resource, FileAccessor& accessor)
		: mResource(std::move(resource))
		, mAccessor(accessor)
	{
		std::unique_lock<std::mutex> lock(mAccessor.mutex);
		mAccessor.condition.wait(lock, [this]
		{
			return mAccessor.status == FileStatus::Available;
		});
		mAccessor.status = FileStatus::Writing;
	}

	~WriteAccessor()
	{
		mAccessor.status = FileStatus::Available;
		mAccessor.condition.notify_all();
	}

	WriteAccessor(const WriteAccessor&) = delete;
	WriteAccessor& operator=(const WriteAccessor&) = delete;

	WriteAccessor(WriteAccessor&&) = default;
	WriteAccessor& operator=(WriteAccessor&&) = default;

	T* operator->() { return &mResource; }
	T& operator*() { return mResource; }
	T& Get() { return mResource; }

	const T* operator->() const { return &mResource; }
	const T& operator*() const { return mResource; }
	const T& Get() const { return mResource; }

private:
	T mResource;
	FileAccessor& mAccessor;
};


class DirectoryEntry : public Path
{
public:

	DirectoryEntry(Path&& path, bool isDirectory)
		: Path(eastl::move(path))
		, mIsDirectory(isDirectory)
	{

	}

	bool IsDirectory() const { return mIsDirectory; }

private:

	bool mIsDirectory = false;

};

class Filesystem
{
public:
    using DirectoryFn = std::function<void(const DirectoryEntry& node)>;

    static void ForEach(const Path& path, const DirectoryFn& fn, bool recursive);
    
	static WriteAccessor<File> Create(const Path& path, FileType type);

	static ReadAccessor<File> OpenRead(const Path& path, FileType type);

	static WriteAccessor<File> OpenWrite(const Path& path, FileType type);
    
    static bool Remove(const Path& path);

	static Path WorkingDirectory();

	static Path Relative(const Path& path, const Path& base);

	static bool Exists(const Path& path);

	static bool IsDirectory(const Path& path);
    
private:

	static FileAccessor& Accessor(const Path& path);
    
    static inline std::mutex mFileCreateMutex;
    
    static inline HashMap<Path, FileAccessor> mFileAccessors;

};

} // namespace Gleam
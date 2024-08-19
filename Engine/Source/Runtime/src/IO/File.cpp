#include "gpch.h"
#include "File.h"

using namespace Gleam;

File::File(FileStream&& handle, const Filesystem::Path& path, FileAccessor& accessor)
	: mName(path.filename().string())
    , mFullPath(path)
	, mHandle(std::move(handle))
    , mAccessor(accessor)
{
	
}

TString File::Read() const
{
	if (not mHandle.is_open())
	{
		GLEAM_CORE_ERROR("File {0} could not be opened.", GetName());
		return "";
	}

    std::unique_lock<std::mutex> lock(mAccessor.mutex);
    mAccessor.condition.wait(lock, [this]
    {
        return mAccessor.status != FileStatus::Writing;
    });
    
    ++mAccessor.concurrentReaders;
    mAccessor.status = FileStatus::Reading;
    lock.unlock();
    
	size_t size = GetSize();
    TString contents;
	contents.resize(size);
	mHandle.read(contents.data(), size);
    
    lock.lock();
    if (--mAccessor.concurrentReaders == 0)
    {
        mAccessor.status = FileStatus::Available;
        mAccessor.condition.notify_all();
    }
	return contents;
}

void File::Write(const TString& contents)
{
	if (not mHandle.is_open())
	{
		GLEAM_CORE_ERROR("File {0} could not be opened.", GetName());
		return;
	}
    
    std::unique_lock<std::mutex> lock(mAccessor.mutex);
    mAccessor.condition.wait(lock, [this] 
    {
        return mAccessor.status == FileStatus::Available;
    });
    
    mAccessor.status = FileStatus::Writing;
    mHandle << contents;
    mAccessor.status = FileStatus::Available;
    
    mAccessor.condition.notify_all();
}

void File::Append(const TString& contents)
{
	if (not mHandle.is_open())
	{
		GLEAM_CORE_ERROR("File {0} could not be opened.", GetName());
		return;
	}
    
    std::unique_lock<std::mutex> lock(mAccessor.mutex);
    mAccessor.condition.wait(lock, [this]
    {
        return mAccessor.status == FileStatus::Available;
    });
    
    mAccessor.status = FileStatus::Writing;
    mHandle.seekg(0, std::ios::end);
    mHandle << contents;
    mAccessor.status = FileStatus::Available;
    
    mAccessor.condition.notify_all();
}

size_t File::GetSize() const
{
    if (mHandle.is_open())
    {
        std::unique_lock<std::mutex> lock(mAccessor.mutex);
        mAccessor.condition.wait(lock, [this]
        {
            return mAccessor.status != FileStatus::Writing;
        });
        
        mHandle.seekg(0, std::ios::end);
        size_t size = mHandle.tellg();
        mHandle.seekg(0, std::ios::beg);
        return size;
    }
    return 0;
}

bool File::Empty() const
{
	return GetSize() == 0;
}

const TString& File::GetName() const
{
	return mName;
}

FileStream& File::GetStream()
{
    return mHandle;
}

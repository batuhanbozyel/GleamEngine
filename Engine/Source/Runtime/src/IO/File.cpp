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
		GLEAM_CORE_ERROR("File could not be opened: {0}", GetName());
		return "";
	}

	FileAccessor::Read accessor(mAccessor);

	size_t size = GetSize();
    TString contents;
	contents.resize(size);
	mHandle.read(contents.data(), size);

	return contents;
}

void File::Write(const TString& contents)
{
	if (not mHandle.is_open())
	{
		GLEAM_CORE_ERROR("File could not be opened: {0}", GetName());
		return;
	}

	FileAccessor::Write accessor(mAccessor);
    mHandle << contents;
}

void File::Append(const TString& contents)
{
	if (not mHandle.is_open())
	{
		GLEAM_CORE_ERROR("File could not be opened: {0}", GetName());
		return;
	}
    
	FileAccessor::Write accessor(mAccessor);
    mHandle.seekg(0, std::ios::end);
    mHandle << contents;
}

size_t File::GetSize() const
{
    if (mHandle.is_open())
    {
		FileAccessor::Read accessor(mAccessor);
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
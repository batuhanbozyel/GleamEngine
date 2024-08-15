#include "gpch.h"
#include "File.h"

using namespace Gleam;

File File::Create(const Filesystem::Path& path, FileType type)
{
    File file(path, type);
    file.CreateIfNotExists();
    return file;
}

File::File(const Filesystem::Path& path, FileType type)
	: mName(path.filename().string())
    , mFullPath(path)
    , mType(type)
{
	auto flags = std::ios::out | std::ios::in;
	if (type == FileType::Binary)
	{
		flags |= std::ios::binary;
	}
	mHandle.open(path, flags);
	mHandle.unsetf(std::ios::skipws);
}

TString File::Read() const
{
	if (not mHandle.is_open())
	{
		GLEAM_CORE_ERROR("File {0} could not be opened.", GetName());
		return "";
	}
    
	size_t size = GetSize();
    
    TString contents;
	contents.resize(size);
	mHandle.read(contents.data(), size);
	return contents;
}

void File::Write(const TString& contents)
{
	CreateIfNotExists();
	mHandle << contents;
}

void File::Append(const TString& contents)
{
	CreateIfNotExists();
	mHandle.seekg(0, std::ios::end);
	Write(contents);
}

void File::CreateIfNotExists()
{
	if (not mHandle.is_open())
	{
		auto flags = std::ios::out | std::ios::in | std::ios::app;
		if (mType == FileType::Binary)
		{
			flags |= std::ios::binary;
		}
		mHandle.open(mFullPath, flags);
		mHandle.unsetf(std::ios::skipws);
	}
}

size_t File::GetSize() const
{
    if (mHandle.is_open())
    {
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

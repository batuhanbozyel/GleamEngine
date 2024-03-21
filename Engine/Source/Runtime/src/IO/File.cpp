#include "gpch.h"
#include "File.h"

using namespace Gleam;

File::File(const Filesystem::path& path, FileType type)
	: mName(path.filename())
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
		GLEAM_CORE_ERROR("File {0} could not be opened!", mName);
		return "";
	}

	mHandle.seekg(0, std::ios::end);
	size_t size = mHandle.tellg();

	TString contents;
	contents.resize(size);

	mHandle.seekg(0, std::ios::beg);
	mHandle.read(contents.data(), size);

	return contents;
}

void File::Write(const TString& contents)
{
	if (not mHandle.is_open())
	{
		GLEAM_CORE_ERROR("File {0} could not be opened!", mName);
		return;
	}
}

const TString& File::GetName() const
{
	return mName;
}
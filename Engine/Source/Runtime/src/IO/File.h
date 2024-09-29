#pragma once
#include "Filesystem.h"

namespace Gleam {

enum class FileType
{
	Binary,
	Text
};

class File final
{
public:

    File(FileStream&& handle, const Filesystem::Path& path, FileAccessor& accessor);

	TString Read() const;

	void Write(const TString& contents);

	void Append(const TString& contents);

	const TString& GetName() const;
    
	FileStream& GetStream();
    
    size_t GetSize() const;

	bool Empty() const;

private:

	TString mName;
    
    Filesystem::Path mFullPath;
    
    FileAccessor& mAccessor;

	mutable FileStream mHandle;
    
};

} // namespace Gleam

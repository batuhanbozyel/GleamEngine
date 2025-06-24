#pragma once
#include "Filesystem.h"
#include "Container/String.h"

namespace Gleam {

enum class FileType
{
	Binary,
	Text
};

class File final
{
public:

    File(FileStream&& handle, const Path& path, FileAccessor& accessor);

	TString Read() const;

	void Write(const TString& contents);

	void Append(const TString& contents);

	const TString& GetName() const;
    
	FileStream& GetStream();
    
    size_t GetSize() const;

	bool Empty() const;
	
	bool IsOpen() const;

private:

	TString mName;
    
    Path mFullPath;
    
    FileAccessor& mAccessor;

	mutable FileStream mHandle;
    
};

} // namespace Gleam

#pragma once
#include "Filesystem.h"

#include <fstream>

namespace Gleam {

using FileStream = std::fstream;

enum class FileType
{
	Binary,
	Text
};

class File final
{
public:
    
    File(FileStream&& handle, const Filesystem::Path& path);

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

	mutable FileStream mHandle;
    
};

} // namespace Gleam

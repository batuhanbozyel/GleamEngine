#pragma once
#include "Filesystem.h"

#include <fstream>

namespace Gleam {

enum class FileType
{
	Binary,
	Text
};

class File final
{
public:
    
    File(const Filesystem::path& path, FileType type);

	TString Read() const;

	void Write(const TString& contents);

	const TString& GetName() const;

private:

	TString mName;
    
    FileType mType;
    
    Filesystem::path mFullPath;

	mutable std::fstream mHandle;
    
};

} // namespace Gleam

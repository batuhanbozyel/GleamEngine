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
    
    File(const Filesystem::path& path, FileType type);

	TString Read() const;

	void Write(const TString& contents);

	void Append(const TString& contents);

	const TString& GetName() const;
    
	FileStream& GetStream();
    
    size_t GetSize() const;
    
    static File Create(const Filesystem::path& path, FileType type);

private:

	void CreateIfNotExists();

	TString mName;
    
    FileType mType;
    
    Filesystem::path mFullPath;

	mutable FileStream mHandle;
    
};

} // namespace Gleam

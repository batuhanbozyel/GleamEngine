#pragma once
#include "Filesystem.h"
#include "Container/String.h"
#include "Container/Pointer.h"

namespace Gleam {

class JSONSerializer;
class BinarySerializer;

enum class FileType
{
	Binary,
	Text
};

class File final
{
	friend class JSONSerializer;
	friend class BinarySerializer;
public:

    File(FileStream&& handle, const Path& path, FileAccessor& accessor);

	~File();

	File(const File&) = delete;
	File& operator=(const File&) = delete;

	File(File&&) = default;
	File& operator=(File&& other) = default;

	TString Read() const;

	void Write(const TString& contents);

	void Append(const TString& contents);

	const TString& GetName() const;
    
    size_t GetSize() const;

	bool Empty() const;
	
	bool IsOpen() const;

	// Unsafe
	FileStream& GetStream() const;

private:

	TString mName;
    
    Path mFullPath;
    
    Ref<FileAccessor> mAccessor;

	mutable FileStream mHandle;
    
};

} // namespace Gleam

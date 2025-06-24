#pragma once
#include <Reflection/Macro.h>
#include "Container/Hash.h"

#include <mutex>
#include <fstream>
#include <functional>
#include <filesystem>

namespace Gleam {

class File;
enum class FileType;
using FileStream = std::fstream;
using Path = std::filesystem::path;

enum class FileStatus
{
    Available,
    Writing,
    Reading
};

struct FileAccessor
{
    std::mutex mutex;
    std::condition_variable condition;
    std::atomic<uint32_t> concurrentReaders = 0;
    FileStatus status = FileStatus::Available;

	struct Write
	{
		FileAccessor& mAccessor;

		Write(FileAccessor& accessor);
		~Write();
	};

	struct Read
	{
		FileAccessor& mAccessor;
		std::unique_lock<std::mutex> mLock;

		Read(FileAccessor& accessor);
		~Read();
	};
};

class Filesystem
{
public:
    using DirectoryFn = std::function<void(const Path& node)>;
    
    static void ForEach(const Path& path, const DirectoryFn& fn, bool recursive);
    
	static File Create(const Path& path, FileType type);

	static File Open(const Path& path, FileType type);
    
    static bool Remove(const Path& path);

	static FileAccessor::Read ReadAccessor(const Path& path);

	static FileAccessor::Write WriteAccessor(const Path& path);

	static Path WorkingDirectory();

	static Path Relative(const Path& path, const Path& base);

	static bool Exists(const Path& path);

	static bool IsDirectory(const Path& path);
    
private:

	static FileAccessor& Accessor(const Path& path);
    
    static inline std::mutex mFileCreateMutex;
    
    static inline HashMap<Path, FileAccessor> mFileAccessors;

};

} // namespace Gleam

namespace Gleam::Reflection::External {

GCLASS(Path, "DA162505-C36B-4FF1-BCB7-FE8428606223", Serializable)
{

};

} // namespace Gleam::Reflection::External
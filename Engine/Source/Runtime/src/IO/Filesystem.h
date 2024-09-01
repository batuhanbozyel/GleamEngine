#pragma once
#include <mutex>
#include <fstream>
#include <functional>
#include <filesystem>

#ifndef _MSC_VER
namespace std {

template <>
struct hash<filesystem::path>
{
    size_t operator()(const filesystem::path& path) const
    {
        return hash_value(path);
    }
};

} // namespace std
#endif

namespace Gleam {

class File;
enum class FileType;
using FileStream = std::fstream;

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
};

class Filesystem
{
public:
	using Path = std::filesystem::path;
    using DirectoryFn = std::function<void(const Path& node)>;
    
    static void ForEach(const Path& path, const DirectoryFn& fn, bool recursive);
    
	static File Create(const Filesystem::Path& path, FileType type);

	static File Open(const Filesystem::Path& path, FileType type);
    
    static bool Remove(const Filesystem::Path& path);

	static FileAccessor& Accessor(const Filesystem::Path& path);

	static Path WorkingDirectory();

	static Path Relative(const Path& path, const Path& base);

	static bool Exists(const Path& path);

	static bool IsDirectory(const Path& path);
    
private:
    
    static inline std::mutex mFileCreateMutex;
    
    static inline HashMap<Filesystem::Path, FileAccessor> mFileAccessors;

};

} // namespace Gleam

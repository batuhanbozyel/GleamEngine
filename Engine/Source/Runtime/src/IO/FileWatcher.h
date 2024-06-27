#pragma once
#include "Core/Subsystem.h"

#include <thread>

namespace Gleam {

enum class FileWatchEvent
{
    Added,
    Removed,
    Renamed,
    Modified
};

using FileWatchHandler = std::function<void(const Filesystem::path&, FileWatchEvent)>;

class FileWatcher final : public Subsystem
{
public:
    
    virtual void Initialize() override;
    
    virtual void Shutdown() override;
    
    void AddWatch(const Filesystem::path& dir, FileWatchHandler&& handler);
    
    void RemoveWatch(const Filesystem::path& dir);
    
private:
    
    struct Watcher;
    HashMap<Filesystem::path, Watcher*> mWatchers;
    
};

} // namespace Gleam

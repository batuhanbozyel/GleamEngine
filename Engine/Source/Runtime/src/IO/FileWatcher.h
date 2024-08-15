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

using FileWatchHandler = std::function<void(const Filesystem::Path&, FileWatchEvent)>;

class FileWatcher final : public Subsystem
{
public:
    
    virtual void Initialize() override;
    
    virtual void Shutdown() override;
    
    void AddWatch(const Filesystem::Path& dir, FileWatchHandler&& handler);
    
    void RemoveWatch(const Filesystem::Path& dir);
    
private:
    
    struct Watcher;
    HashMap<Filesystem::Path, Watcher*> mWatchers;
    
};

} // namespace Gleam

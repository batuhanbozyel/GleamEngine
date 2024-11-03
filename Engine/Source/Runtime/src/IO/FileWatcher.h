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

class FileWatcher final : public EngineSubsystem
{
	struct Handle;
public:
    
    virtual void Initialize(Engine* engine) override;
    
    virtual void Shutdown() override;
    
	Handle* AddWatch(const Filesystem::Path& dir, FileWatchHandler&& handler);
    
    void RemoveWatch(Handle* watcher);
    
private:
    
    HashMap<Filesystem::Path, TArray<Handle*>> mWatchers;
    
};

} // namespace Gleam

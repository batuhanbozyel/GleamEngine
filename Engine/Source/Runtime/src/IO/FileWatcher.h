#pragma once
#include "Core/Subsystem.h"
#include "IO/Filesystem.h"
#include "Container/Array.h"

#include <functional>

namespace Gleam {

enum class FileWatchEvent
{
    Added,
    Removed,
    Renamed,
    Modified
};

using FileWatchHandler = std::function<void(const Path&, FileWatchEvent)>;

class FileWatcher final : public EngineSubsystem
{
	struct Handle;
public:
    
    virtual void Initialize(Engine* engine) override;
    
    virtual void Shutdown() override;
    
	Handle* AddWatch(const Path& dir, FileWatchHandler&& handler);
    
    void RemoveWatch(Handle* watcher);
    
private:
    
    HashMap<Path, TArray<Handle*>> mWatchers;
    
};

} // namespace Gleam

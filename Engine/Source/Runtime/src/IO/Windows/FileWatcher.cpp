#include "gpch.h"

#ifdef PLATFORM_WINDOWS
#include "IO/FileWatcher.h"

#include <Windows.h>

using namespace Gleam;

void FileWatcher::Initialize()
{
    
}

void FileWatcher::Shutdown()
{
    for (auto& [_, watcher] : mWatchers)
    {
        delete watcher;
    }
}

void FileWatcher::AddWatch(const Filesystem::path& dir, FileWatchHandler&& handler)
{
    if (mWatchers.find(dir) != mWatchers.end())
    {
        RemoveWatch(dir);
    }
}

void FileWatcher::RemoveWatch(const Filesystem::path& dir)
{
    auto it = mWatchers.find(dir);
    if (it == mWatchers.end())
    {
        return;
    }
    
    Watcher* watcher = it->second;
    mWatchers.erase(it);
    delete watcher;
}

#endif


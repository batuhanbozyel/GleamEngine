#include "gpch.h"

#ifdef PLATFORM_MACOS
#include "IO/FileWatcher.h"

#import <AppKit/AppKit.h>
#import <CoreServices/CoreServices.h>

using namespace Gleam;

struct FileWatcher::Watcher
{
    Filesystem::Path path;
    FileWatchHandler handler;
    FSEventStreamRef _Nonnull stream;
 
    Watcher(const Filesystem::Path& path, FileWatchHandler&& handler)
        : path(path), handler(std::move(handler)), stream(nullptr)
    {
        
    }
    
    ~Watcher()
    {
        FSEventStreamStop(stream);
        FSEventStreamInvalidate(stream);
        FSEventStreamRelease(stream);
    }
    
    static void CallbackHandler(ConstFSEventStreamRef _Nonnull streamRef,
                                void* __nullable clientCallBackInfo,
                                size_t numEvents,
                                void* _Nonnull eventPaths,
                                const FSEventStreamEventFlags* _Nonnull eventFlags,
                                const FSEventStreamEventId* _Nonnull eventIds)
    {
        auto* watcher = static_cast<Watcher*>(clientCallBackInfo);
        for (size_t i = 0; i < numEvents; ++i)
        {
            Filesystem::Path path = ((const char**)eventPaths)[i];
            const FSEventStreamEventFlags flags = eventFlags[i];
            if (!(flags & kFSEventStreamEventFlagItemIsFile))
            {
                // TODO: implement support for subdirectories
                if (flags & kFSEventStreamEventFlagItemRemoved)
                {
                    watcher->handler(path, FileWatchEvent::Removed);
                    break;
                }
            }
            else if (flags & kFSEventStreamEventFlagItemCreated)
            {
                watcher->handler(path, FileWatchEvent::Added);
            }
            else if (flags & kFSEventStreamEventFlagItemRemoved)
            {
                watcher->handler(path, FileWatchEvent::Removed);
            }
            else if (flags & (kFSEventStreamEventFlagItemRenamed))
            {
                watcher->handler(path, FileWatchEvent::Renamed);
            }
            else if (flags & (kFSEventStreamEventFlagItemModified))
            {
                watcher->handler(path, FileWatchEvent::Modified);
            }
            // default
            else
            {
                GLEAM_ASSERT(false, "Apple: Unknown FSEvent type.");
            }
        }
    }
};

void FileWatcher::Initialize(Engine* engine)
{
    
}

void FileWatcher::Shutdown()
{
    for (auto& [_, watcher] : mWatchers)
    {
        delete watcher;
    }
    mWatchers.clear();
}

void FileWatcher::AddWatch(const Filesystem::Path& dir, FileWatchHandler&& handler)
{
    if (Filesystem::IsDirectory(dir) == false)
	{
        GLEAM_CORE_ERROR("FileWatcher requires directory: {0}", dir.string());
		return;
	}

    if (mWatchers.find(dir) != mWatchers.end())
    {
        RemoveWatch(dir);
    }
    
    CFStringRef pathCF = CFStringCreateWithCString(NULL, dir.c_str(), kCFStringEncodingUTF8);
    CFArrayRef pathsCF = CFArrayCreate(NULL, (const void**)&pathCF, 1, NULL);
    
    Watcher* watcher = new Watcher(dir, std::forward<FileWatchHandler>(handler));
    mWatchers[dir] = watcher;
    
    FSEventStreamContext context;
    context.version = 0;
    context.info = watcher;
    context.retain = NULL;
    context.release = NULL;
    context.copyDescription = NULL;
    
    watcher->stream = FSEventStreamCreate(NULL,
                                          &Watcher::CallbackHandler,
                                          &context,
                                          pathsCF,
                                          kFSEventStreamEventIdSinceNow,
                                          0.0,
                                          kFSEventStreamCreateFlagFileEvents | kFSEventStreamCreateFlagNoDefer);
    
    dispatch_queue_t queue = dispatch_queue_create("FileWatcher", DISPATCH_QUEUE_SERIAL);
    FSEventStreamSetDispatchQueue(watcher->stream, queue);
    FSEventStreamStart(watcher->stream);
    
    CFRelease(pathsCF);
    CFRelease(pathCF);
}

void FileWatcher::RemoveWatch(const Filesystem::Path& dir)
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


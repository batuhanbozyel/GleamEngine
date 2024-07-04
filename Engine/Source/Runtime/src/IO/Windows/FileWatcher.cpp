#include "gpch.h"

#ifdef PLATFORM_WINDOWS
#include "IO/FileWatcher.h"

#include <WinBase.h>

using namespace Gleam;

struct FileWatcher::Watcher
{
	Filesystem::path path;
	FileWatchHandler handler;

	BYTE buffer[1024];
	HANDLE win32Handle;
	OVERLAPPED overlapped;
	std::thread watchThread;
	std::atomic<bool> running;
	std::condition_variable condition;

	Watcher(const Filesystem::path& path, FileWatchHandler&& handler, HANDLE dirHandle)
		: path(path), handler(std::move(handler)), win32Handle(dirHandle), running(true)
	{
		memset(&overlapped, 0, sizeof(OVERLAPPED));
		memset(&buffer, 0, sizeof(BYTE) * _countof(buffer));

		overlapped.hEvent = this;
		watchThread = std::thread([this]()
		{
			while (running)
			{
				DWORD bytesReturned;
				if (::ReadDirectoryChangesW(win32Handle, buffer, sizeof(buffer), TRUE,
					FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME |
					FILE_NOTIFY_CHANGE_ATTRIBUTES | FILE_NOTIFY_CHANGE_SIZE |
					FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_CREATION,
					&bytesReturned, &overlapped, &CallbackHandler))
				{
					SleepEx(INFINITE, TRUE);
				}
				else
				{
					running = false;
				}
			}
		});
	}

	~Watcher()
	{
		::CancelIo(win32Handle);
		if (watchThread.joinable())
		{
			::QueueUserAPC(TerminateProc, reinterpret_cast<HANDLE>(watchThread.native_handle()), (ULONG_PTR)this);
			watchThread.join();
		}
		::CloseHandle(win32Handle);
	}

	// Called by QueueUserAPC to start orderly shutdown.
	static void CALLBACK TerminateProc(__in  ULONG_PTR arg)
	{
		Watcher* watcher = (Watcher*)arg;
		watcher->running = false;
	}

	static void CALLBACK CallbackHandler(DWORD errorCode, DWORD bytesTransferred, LPOVERLAPPED lpOverlapped)
	{
		Watcher* watcher = static_cast<Watcher*>(lpOverlapped->hEvent);
		if (errorCode != ERROR_SUCCESS)
		{
			watcher->running = false;
			return;
		}

		FILE_NOTIFY_INFORMATION* info = (FILE_NOTIFY_INFORMATION*)watcher->buffer;
		while (true)
		{
			Filesystem::path filepath = watcher->path / TWString(info->FileName, info->FileNameLength / sizeof(WCHAR));
			switch (info->Action)
			{
				case FILE_ACTION_ADDED:
				{
					GLEAM_CORE_INFO("FileWatcher file added event: {}", filepath.string());
					watcher->handler(filepath, FileWatchEvent::Added);
					break;
				}
				case FILE_ACTION_REMOVED:
				{
					GLEAM_CORE_INFO("FileWatcher file removed event: {}", filepath.string());
					watcher->handler(filepath, FileWatchEvent::Removed);
					break;
				}
				case FILE_ACTION_MODIFIED:
				{
					GLEAM_CORE_INFO("FileWatcher file modified event: {}", filepath.string());
					watcher->handler(filepath, FileWatchEvent::Modified);
					break;
				}
				case FILE_ACTION_RENAMED_OLD_NAME:
				case FILE_ACTION_RENAMED_NEW_NAME:
				{
					GLEAM_CORE_INFO("FileWatcher file renamed event: {}", filepath.string());
					watcher->handler(filepath, FileWatchEvent::Renamed);
					break;
				}
			}

			if (info->NextEntryOffset == 0)
			{
				break;
			}
			info = (FILE_NOTIFY_INFORMATION*)((BYTE*)info + info->NextEntryOffset);
		}

		// reinitialize
		watcher->condition.notify_one();
	}
};

void FileWatcher::Initialize()
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

void FileWatcher::AddWatch(const Filesystem::path& dir, FileWatchHandler&& handler)
{
	if (Filesystem::is_directory(dir) == false)
	{
		GLEAM_CORE_ERROR("FileWatcher requires directory: {0}", dir.string());
		return;
	}

    if (mWatchers.find(dir) != mWatchers.end())
    {
        RemoveWatch(dir);
    }

	HANDLE dirHandle = ::CreateFileA(dir.string().c_str(),
		FILE_LIST_DIRECTORY,
		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		nullptr,
		OPEN_EXISTING,
		FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
		nullptr);

	if (dirHandle == INVALID_HANDLE_VALUE)
	{
		GLEAM_CORE_ERROR("FileWatcher Win32 handle creation failed: {0}", dir.string());
		return;
	}

	Watcher* watcher = new Watcher(dir, std::forward<FileWatchHandler>(handler), dirHandle);
	mWatchers[dir] = watcher;
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
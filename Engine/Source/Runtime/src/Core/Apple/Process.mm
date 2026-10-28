#include "gpch.h"

#ifdef PLATFORM_MACOS
#include "Core/Process.h"

#include <spawn.h>
#import <AppKit/AppKit.h>
#include <mach-o/dyld.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>

extern char** environ;

using namespace Gleam;

// Signal handlers take no arguments, so everything the relaunch needs is prepared up front
class CrashRelaunch final
{
public:

	static void Install(const Path& crashLog, const TArray<TString>& arguments)
	{
		mExecutable = Process::ExecutablePath().String();
		mLogFile = (Filesystem::WorkingDirectory() / Logger::FILE_NAME).String();
		mCrashLogFile = crashLog.String();
		mArguments = arguments;

		mArgv.push_back(mExecutable.data());
		for (auto& argument : mArguments)
		{
			mArgv.push_back(argument.data());
		}
		mArgv.push_back(nullptr);

		struct sigaction action = {};
		action.sa_handler = HandleSignal;
		// The relaunched image inherits the signal mask, so the crashing signal must not stay blocked
		action.sa_flags = SA_NODEFER | SA_RESETHAND;
		sigemptyset(&action.sa_mask);
		for (const auto signal : { SIGSEGV, SIGBUS, SIGILL, SIGFPE, SIGABRT, SIGTRAP })
		{
			sigaction(signal, &action, nullptr);
		}
	}

private:

	static const char* SignalName(int signal)
	{
		switch (signal)
		{
			case SIGSEGV: return "SIGSEGV";
			case SIGBUS: return "SIGBUS";
			case SIGILL: return "SIGILL";
			case SIGFPE: return "SIGFPE";
			case SIGABRT: return "SIGABRT";
			case SIGTRAP: return "SIGTRAP";
			default: return "unknown signal";
		}
	}

	static void Write(int file, const char* text)
	{
		write(file, text, strlen(text));
	}

	// Only async signal safe calls are made here
	static void HandleSignal(int signal)
	{
		const auto target = open(mCrashLogFile.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
		if (target != -1)
		{
			if (const auto source = open(mLogFile.c_str(), O_RDONLY); source != -1)
			{
				char buffer[4096];
				for (auto bytesRead = read(source, buffer, sizeof(buffer)); bytesRead > 0; bytesRead = read(source, buffer, sizeof(buffer)))
				{
					write(target, buffer, static_cast<size_t>(bytesRead));
				}
				close(source);
			}
			Write(target, "[fatal] GLEAM: Crashed with ");
			Write(target, SignalName(signal));
			Write(target, "\n");
			close(target);
		}

		execve(mExecutable.c_str(), mArgv.data(), environ);

		// The relaunch failed, the signal terminates the process as it would without the handler
		raise(signal);
	}

	static inline TString mExecutable;
	static inline TString mLogFile;
	static inline TString mCrashLogFile;
	static inline TArray<TString> mArguments;
	static inline TArray<char*> mArgv;
};

bool Process::Launch(const Path& executable, const TArray<TString>& arguments, const Path& workingDirectory)
{
	const auto executableStr = executable.String();
	const auto workingDirectoryStr = workingDirectory.String();

	TArray<char*> argv;
	argv.push_back(const_cast<char*>(executableStr.c_str()));
	for (const auto& argument : arguments)
	{
		argv.push_back(const_cast<char*>(argument.c_str()));
	}
	argv.push_back(nullptr);

	posix_spawn_file_actions_t actions;
	posix_spawn_file_actions_init(&actions);
	posix_spawn_file_actions_addchdir(&actions, workingDirectoryStr.c_str());

	pid_t pid = 0;
	int result = posix_spawn(&pid, executableStr.c_str(), &actions, nullptr, argv.data(), environ);
	posix_spawn_file_actions_destroy(&actions);
	return result == 0;
}

Path Process::ExecutablePath()
{
	uint32_t size = 0;
	_NSGetExecutablePath(nullptr, &size);
	TString buffer(size, '\0');
	_NSGetExecutablePath(buffer.data(), &size);

	char resolved[PATH_MAX];
	return Path(realpath(buffer.c_str(), resolved) ? resolved : buffer.c_str());
}

void Process::RevealInFileBrowser(const Path& path)
{
	NSString* pathString = [NSString stringWithUTF8String:path.String().c_str()];
	[[NSWorkspace sharedWorkspace] activateFileViewerSelectingURLs:@[[NSURL fileURLWithPath:pathString]]];
}

void Process::OpenWithDefaultApplication(const Path& path)
{
	NSString* pathString = [NSString stringWithUTF8String:path.String().c_str()];
	[[NSWorkspace sharedWorkspace] openURL:[NSURL fileURLWithPath:pathString]];
}

void Process::RelaunchOnCrash(const Path& crashLog, const TArray<TString>& arguments)
{
	CrashRelaunch::Install(crashLog, arguments);
}

#endif

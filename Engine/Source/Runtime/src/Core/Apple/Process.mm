#include "gpch.h"

#ifdef PLATFORM_MACOS
#include "Core/Process.h"

#include <spawn.h>
#include <mach-o/dyld.h>

extern char** environ;

using namespace Gleam;

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
	posix_spawn_file_actions_addchdir_np(&actions, workingDirectoryStr.c_str());

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

#endif

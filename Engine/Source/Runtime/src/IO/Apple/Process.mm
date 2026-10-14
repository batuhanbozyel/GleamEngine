#include "gpch.h"

#ifdef PLATFORM_MACOS
#include "IO/Process.h"

#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>

extern char** environ;

using namespace Gleam;

ProcessResult Gleam::RunProcess(const TStringView commandLine)
{
	ProcessResult result;

	int pipeFds[2];
	if (pipe(pipeFds) == 0)
	{
		posix_spawn_file_actions_t fileActions;
		posix_spawn_file_actions_init(&fileActions);
		posix_spawn_file_actions_addclose(&fileActions, pipeFds[0]);
		posix_spawn_file_actions_adddup2(&fileActions, pipeFds[1], STDOUT_FILENO);
		posix_spawn_file_actions_adddup2(&fileActions, pipeFds[1], STDERR_FILENO);
		posix_spawn_file_actions_addclose(&fileActions, pipeFds[1]);

		TString command(commandLine);
		char shell[] = "/bin/sh";
		char flag[] = "-c";
		char* argv[] = { shell, flag, command.data(), nullptr };

		pid_t pid = 0;
		int spawnResult = posix_spawn(&pid, shell, &fileActions, nullptr, argv, environ);
		posix_spawn_file_actions_destroy(&fileActions);
		close(pipeFds[1]);

		if (spawnResult == 0)
		{
			char buffer[4096];
			ssize_t bytesRead = 0;
			while ((bytesRead = read(pipeFds[0], buffer, sizeof(buffer))) > 0)
			{
				result.output.append(buffer, static_cast<size_t>(bytesRead));
			}

			int status = 0;
			if (waitpid(pid, &status, 0) == pid and WIFEXITED(status))
			{
				result.exitCode = WEXITSTATUS(status);
			}
		}
		close(pipeFds[0]);
	}
	return result;
}
#endif

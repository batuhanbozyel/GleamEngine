#include "gpch.h"

#ifdef PLATFORM_WINDOWS
#include "IO/Process.h"

using namespace Gleam;

ProcessResult Gleam::RunProcess(const TStringView commandLine)
{
	ProcessResult result;

	SECURITY_ATTRIBUTES securityAttributes{ .nLength = sizeof(SECURITY_ATTRIBUTES), .bInheritHandle = TRUE };
	HANDLE readPipe = nullptr;
	HANDLE writePipe = nullptr;
	if (CreatePipe(&readPipe, &writePipe, &securityAttributes, 0))
	{
		SetHandleInformation(readPipe, HANDLE_FLAG_INHERIT, 0);

		STARTUPINFOW startupInfo{ .cb = sizeof(STARTUPINFOW) };
		startupInfo.dwFlags = STARTF_USESTDHANDLES;
		startupInfo.hStdOutput = writePipe;
		startupInfo.hStdError = writePipe;

		TWString command = StringUtils::Convert(commandLine);
		PROCESS_INFORMATION processInfo{};
		BOOL created = CreateProcessW(nullptr, command.data(), nullptr, nullptr, TRUE, CREATE_NO_WINDOW, nullptr, nullptr, &startupInfo, &processInfo);
		CloseHandle(writePipe);

		if (created)
		{
			char buffer[4096];
			DWORD bytesRead = 0;
			while (ReadFile(readPipe, buffer, sizeof(buffer), &bytesRead, nullptr) and bytesRead > 0)
			{
				result.output.append(buffer, bytesRead);
			}

			WaitForSingleObject(processInfo.hProcess, INFINITE);
			DWORD exitCode = 0;
			GetExitCodeProcess(processInfo.hProcess, &exitCode);
			result.exitCode = static_cast<int>(exitCode);

			CloseHandle(processInfo.hThread);
			CloseHandle(processInfo.hProcess);
		}
		CloseHandle(readPipe);
	}
	return result;
}
#endif

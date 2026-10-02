#include "gpch.h"

#ifdef PLATFORM_WINDOWS
#include "Core/Process.h"

#include <Windows.h>

using namespace Gleam;

bool Process::Launch(const Path& executable, const TArray<TString>& arguments, const Path& workingDirectory)
{
	TWString commandLine = L"\"" + executable.Native() + L"\"";
	for (const auto& argument : arguments)
	{
		commandLine += L" \"";
		commandLine.append_convert(argument);
		commandLine += L"\"";
	}

	STARTUPINFOW startupInfo = {};
	startupInfo.cb = sizeof(STARTUPINFOW);
	PROCESS_INFORMATION processInfo = {};
	if (CreateProcessW(executable.Native().c_str(), commandLine.data(), nullptr, nullptr, FALSE, 0, nullptr, workingDirectory.Native().c_str(), &startupInfo, &processInfo) == FALSE)
	{
		return false;
	}

	CloseHandle(processInfo.hThread);
	CloseHandle(processInfo.hProcess);
	return true;
}

Path Process::ExecutablePath()
{
	TWString buffer(MAX_PATH, L'\0');
	DWORD length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
	while (length == buffer.size())
	{
		buffer.resize(buffer.size() * 2);
		length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
	}
	buffer.resize(length);
	return Path(buffer);
}

#endif

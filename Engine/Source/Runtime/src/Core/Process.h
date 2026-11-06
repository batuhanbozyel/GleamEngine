#pragma once
#include "IO/Path.h"
#include "Container/Array.h"
#include "Container/String.h"
#include "Core/Macro.h"

namespace Gleam {

class ChildProcess final
{
	friend class Process;
public:

	GLEAM_NONCOPYABLE(ChildProcess);

	~ChildProcess();

	bool IsRunning();

	bool WaitForExit(uint32_t milliseconds);

	void Terminate();

	uint32_t GetId() const
	{
		return mId;
	}

	int GetExitCode() const
	{
		return mExitCode;
	}

private:

	ChildProcess(intptr_t handle, uint32_t id)
		: mHandle(handle)
		, mId(id)
	{

	}

	intptr_t mHandle = -1;

	uint32_t mId = 0;

	int mExitCode = -1;

	bool mExited = false;

};

class Process final
{
public:

	static ChildProcess* Launch(const Path& executable, const TArray<TString>& arguments, const Path& workingDirectory);

	static Path ExecutablePath();

	static uint32_t CurrentId();

	static void RevealInFileBrowser(const Path& path);

	static void OpenWithDefaultApplication(const Path& path);

	// After a fatal crash the log is copied to crashLog and this executable is started again with the arguments
	static void RelaunchOnCrash(const Path& crashLog, const TArray<TString>& arguments);

};

} // namespace Gleam

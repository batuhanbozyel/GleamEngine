#include "gpch.h"
#include "Log.h"
#include "Core/Globals.h"

#if defined(PLATFORM_WINDOWS)
#include <Windows.h>
#elif defined(PLATFORM_MACOS)
#include <os/log.h>
#endif

using namespace Gleam;

Logger::Logger(const TString& name)
    : mName(name)
{
	mName.append(": ");
    static std::once_flag flag;
    std::call_once(flag, [this]()
    {
        mFileStream = CreateScope<std::ofstream>("Gleam.log", std::ofstream::out);
    });
    mInstanceCount++;
}

Logger::~Logger()
{
    mInstanceCount--;
    if (mInstanceCount == 0)
    {
        mFileStream.reset();
    }
}

const Logger& Logger::GetCoreLogger()
{
	static Logger sLogger("GLEAM");
	return sLogger;
}

const Logger& Logger::GetClientLogger()
{
	static Logger sLogger(Globals::ProjectName);
	return sLogger;
}

void Logger::OutputToDebugger(const char* message)
{
#ifdef PLATFORM_WINDOWS
	::OutputDebugStringA(message);
#elif defined(PLATFORM_MACOS)
	os_log(OS_LOG_DEFAULT, "%{public}s", message);
#else
	std::fputs(message, stdout);
	std::fflush(stdout);
#endif
}

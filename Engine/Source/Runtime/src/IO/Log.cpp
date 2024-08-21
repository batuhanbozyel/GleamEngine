#include "gpch.h"
#include "Log.h"
#include "Core/Globals.h"

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

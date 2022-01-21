#include "gpch.h"
#include "Log.h"

using namespace Gleam;

Logger::Logger(const TStringView name)
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

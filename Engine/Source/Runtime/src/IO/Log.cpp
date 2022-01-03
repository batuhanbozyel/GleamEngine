#include "gpch.h"
#include "Log.h"

#include <fmt/core.h>

using namespace Gleam;

static constexpr TStringView LogLevelToString(Logger::Level lvl)
{
    switch (lvl)
    {
        case Logger::Level::Trace: return "[Trace]";
        case Logger::Level::Info: return "[Info]";
        case Logger::Level::Warn: return "[Warn]";
        case Logger::Level::Error: return "[Error]";
        case Logger::Level::Critical: return "[Critical]";
    }
};

Logger::Logger(const TStringView name)
    : mName(name)
{
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

TString Logger::Log(Level lvl, const TStringView frmt, ...) const
{
    va_list args;
    va_start(args, frmt);
    
    std::ostringstream ss;
    ss << mName << LogLevelToString(lvl) << ' ' << fmt::format(frmt, va_arg(args, const char*)) << '\n';
    
    va_end(args);
    
    TString outString = ss.str();
    *mFileStream << outString;
    std::flush(*mFileStream);
    return outString;
}

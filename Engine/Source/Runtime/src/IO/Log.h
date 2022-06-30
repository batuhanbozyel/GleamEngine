#pragma once
#define FMT_HEADER_ONLY
#include <fmt/core.h>
#include <fstream>
#include <chrono>

namespace Gleam {

class Logger final
{
public:
    
    GLEAM_NONCOPYABLE(Logger);
    
    enum class Level
    {
        Trace,
        Info,
        Warn,
        Error
    };

	Logger(const TStringView name);
    ~Logger();
    
    static const Logger& GetCoreLogger()
    {
        static Logger sLogger("GLEAM");
        return sLogger;
    }
    
    static const Logger& GetClientLogger()
    {
        static Logger sLogger("APP");
        return sLogger;
    }

	template<typename ... Args>
	void Log(Level lvl, const TStringView frmt, Args&& ... args) const
	{
		auto currentTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
		char formattedCurrentTime[32];
		std::strftime(formattedCurrentTime, 32, "%X", std::localtime(&currentTime));

		std::ostringstream ss;
		ss << '[' << formattedCurrentTime << "] ";
		ss << LogLevelToString(lvl) << mName << fmt::format(fmt::runtime(frmt), std::forward<Args>(args)...) << '\n';

		*mFileStream << ss.str();
		std::flush(*mFileStream);
	}
    
private:

	static constexpr TStringView LogLevelToString(Logger::Level lvl)
	{
		switch (lvl)
		{
			case Logger::Level::Trace: return "[trace] ";
			case Logger::Level::Info: return "[info] ";
			case Logger::Level::Warn: return "[warning] ";
			case Logger::Level::Error: return "[error] ";
			default: return "[undefined] ";
		}
	};
    
    TString mName;
    
    static inline Scope<std::ofstream> mFileStream = nullptr;
    static inline uint32_t mInstanceCount = 0;
    
};

} // namespace Gleam

// Core log macros
#define GLEAM_CORE_TRACE(...) ::Gleam::Logger::GetCoreLogger().Log(::Gleam::Logger::Level::Trace, __VA_ARGS__)
#define GLEAM_CORE_INFO(...) ::Gleam::Logger::GetCoreLogger().Log(::Gleam::Logger::Level::Info, __VA_ARGS__)
#define GLEAM_CORE_WARN(...) ::Gleam::Logger::GetCoreLogger().Log(::Gleam::Logger::Level::Warn, __VA_ARGS__)
#define GLEAM_CORE_ERROR(...) ::Gleam::Logger::GetCoreLogger().Log(::Gleam::Logger::Level::Error, __VA_ARGS__)

// Client log macros
#define GLEAM_TRACE(...) ::Gleam::Logger::GetClientLogger().Log(::Gleam::Logger::Level::Trace, __VA_ARGS__)
#define GLEAM_INFO(...) ::Gleam::Logger::GetClientLogger().Log(::Gleam::Logger::Level::Info, __VA_ARGS__)
#define GLEAM_WARN(...) ::Gleam::Logger::GetClientLogger().Log(::Gleam::Logger::Level::Warn, __VA_ARGS__)
#define GLEAM_ERROR(...) ::Gleam::Logger::GetClientLogger().Log(::Gleam::Logger::Level::Error, __VA_ARGS__)

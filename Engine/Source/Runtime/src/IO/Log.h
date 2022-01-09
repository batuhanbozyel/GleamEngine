#pragma once
#include <fmt/core.h>

namespace Gleam {

class Logger
{
public:
    
    enum class Level
    {
        Trace,
        Info,
        Warn,
        Error
    };
    
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
		ss << LogLevelToString(lvl) << mName << fmt::format(frmt, args...) << '\n';

		*mFileStream << ss.str();
		std::flush(*mFileStream);
	}

	Logger(const Logger&) = delete;
	Logger& operator=(const Logger&) = delete;
    
private:
    
    Logger(const TStringView name);
    
    TString mName;
    
    static inline Scope<std::ofstream> mFileStream = nullptr;
    static inline uint32_t mInstanceCount = 0;

	static constexpr TStringView LogLevelToString(Logger::Level lvl)
	{
		switch (lvl)
		{
			case Logger::Level::Trace: return "[trace] ";
			case Logger::Level::Info: return "[info] ";
			case Logger::Level::Warn: return "[warning] ";
			case Logger::Level::Error: return "[error] ";
		}
	};
    
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

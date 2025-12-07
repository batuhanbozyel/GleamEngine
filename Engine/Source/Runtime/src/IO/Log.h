#pragma once
#include "Container/Pointer.h"
#include "Container/String.h"

#define FMT_HEADER_ONLY
#define FMT_UNICODE 0
#include <fmt/core.h>

#include <fstream>
#include <chrono>
#include <mutex>

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

	Logger(const TString& name);
    ~Logger();
    
	static const Logger& GetCoreLogger();

	static const Logger& GetClientLogger();

	template<typename ... Args>
	void Log(Level lvl, const TStringView frmt, Args&& ... args) const
	{
		if (static_cast<uint32_t>(lvl) < static_cast<uint32_t>(mLevel))
		{
			return;
		}

		auto currentTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
		char formattedCurrentTime[32];
		std::strftime(formattedCurrentTime, 32, "%X", std::localtime(&currentTime));

		std::ostringstream ss;
		ss << '[' << formattedCurrentTime << "] ";
		ss << LogLevelToString(lvl) << mName << fmt::format(fmt::runtime(frmt), std::forward<Args>(args)...) << '\n';

		const auto& msg = ss.str();
		{
			std::lock_guard<std::mutex> lock(mLogMutex);
			*mFileStream << msg;
			std::flush(*mFileStream);
			OutputToDebugger(msg.c_str());
		}
	}

	static void SetLevel(Level lvl)
	{
		mLevel = lvl;
	}

private:

	static constexpr TStringView LogLevelToString(Level lvl)
	{
		switch (lvl)
		{
			case Level::Trace: return "[trace] ";
			case Level::Info: return "[info] ";
			case Level::Warn: return "[warning] ";
			case Level::Error: return "[error] ";
			default: return "[undefined] ";
		}
	};

	static void OutputToDebugger(const char* message);
    
    TString mName;
    
    static inline Scope<std::ofstream> mFileStream = nullptr;
    static inline uint32_t mInstanceCount = 0;
	static inline std::mutex mLogMutex;
	static inline Level mLevel = Level::Info;
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

#include <filesystem>
#define GLEAM_ASSERT_IMPL(cond, msg, ...) \
    if (!(cond)) { \
        GLEAM_CORE_ERROR(msg, __VA_ARGS__); \
        DEBUGBREAK(); \
    }
#define GLEAM_ASSERT_WITH_MSG(cond, ...) GLEAM_ASSERT_IMPL(cond, "Assertion failed at {0}:{1}. Reason: {2}", std::filesystem::path(__FILE__).filename().string(),  __LINE__, __VA_ARGS__)
#define GLEAM_ASSERT_NO_MSG(cond) GLEAM_ASSERT_IMPL(cond, "Assertion failed at {0}:{1}", std::filesystem::path(__FILE__).filename().string(),  __LINE__)

#define GLEAM_ASSERT_GET_MACRO_NAME_(n) GLEAM_ASSERT_GET_MACRO_NAME_##n
#define GLEAM_ASSERT_GET_MACRO_NAME(n) GLEAM_ASSERT_GET_MACRO_NAME_(n)

#define GLEAM_ASSERT_GET_MACRO_NAME_1 GLEAM_ASSERT_NO_MSG
#define GLEAM_ASSERT_GET_MACRO_NAME_2 GLEAM_ASSERT_WITH_MSG
#define GLEAM_ASSERT_GET_MACRO_NAME_3 GLEAM_ASSERT_WITH_MSG
#define GLEAM_ASSERT_GET_MACRO_NAME_4 GLEAM_ASSERT_WITH_MSG
#define GLEAM_ASSERT_GET_MACRO_NAME_5 GLEAM_ASSERT_WITH_MSG
#define GLEAM_ASSERT_GET_MACRO_NAME_6 GLEAM_ASSERT_WITH_MSG
#define GLEAM_ASSERT_GET_MACRO_NAME_7 GLEAM_ASSERT_WITH_MSG
#define GLEAM_ASSERT_GET_MACRO_NAME_8 GLEAM_ASSERT_WITH_MSG
#define GLEAM_ASSERT_GET_MACRO_NAME_9 GLEAM_ASSERT_WITH_MSG
#define GLEAM_ASSERT_GET_MACRO(...) GLEAM_EXPAND(GLEAM_ASSERT_GET_MACRO_NAME(GLEAM_FOREACH_NUM_ARGS(__VA_ARGS__)))

#ifdef ENABLE_ASSERTS
#define GLEAM_ASSERT(...) GLEAM_EXPAND(GLEAM_ASSERT_GET_MACRO(__VA_ARGS__)(__VA_ARGS__))
#else
#define GLEAM_ASSERT(...)
#endif

#define GLEAM_AFFIRM(...) GLEAM_EXPAND(GLEAM_ASSERT_GET_MACRO(__VA_ARGS__)(__VA_ARGS__))

static bool ExecuteCommand(const Gleam::TString& cmd)
{
    int success = system((cmd + " > command.err 2>&1").c_str());
    return success == 0;
}

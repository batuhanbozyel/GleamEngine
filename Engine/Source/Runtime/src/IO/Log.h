#pragma once
#include <spdlog/spdlog.h>

namespace Gleam {

class Log
{
public:
	static void Init();

	static Ref<spdlog::logger>& GetCoreLogger() { return sCoreLogger;  }
	static Ref<spdlog::logger>& GetClientLogger() { return sClientLogger; }

private:

	static inline Ref<spdlog::logger> sCoreLogger = nullptr;
	static inline Ref<spdlog::logger> sClientLogger = nullptr;

};

} // namespace Gleam

// Core log macros
#define GLEAM_CORE_TRACE(...)    ::Gleam::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define GLEAM_CORE_INFO(...)     ::Gleam::Log::GetCoreLogger()->info(__VA_ARGS__)
#define GLEAM_CORE_WARN(...)     ::Gleam::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define GLEAM_CORE_ERROR(...)    ::Gleam::Log::GetCoreLogger()->error(__VA_ARGS__)
#define GLEAM_CORE_CRITICAL(...) ::Gleam::Log::GetCoreLogger()->critical(__VA_ARGS__)

// Client log macros
#define GLEAM_TRACE(...)         ::Gleam::Log::GetClientLogger()->trace(__VA_ARGS__)
#define GLEAM_INFO(...)          ::Gleam::Log::GetClientLogger()->info(__VA_ARGS__)
#define GLEAM_WARN(...)          ::Gleam::Log::GetClientLogger()->warn(__VA_ARGS__)
#define GLEAM_ERROR(...)         ::Gleam::Log::GetClientLogger()->error(__VA_ARGS__)
#define GLEAM_CRITICAL(...)      ::Gleam::Log::GetClientLogger()->critical(__VA_ARGS__)
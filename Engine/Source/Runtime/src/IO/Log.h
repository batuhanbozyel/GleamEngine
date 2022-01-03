#pragma once

namespace Gleam {


class Logger
{
public:
    
    enum class Level
    {
        Trace,
        Info,
        Warn,
        Error,
        Critical
    };
    
    ~Logger();
    
    static const Logger& GetCoreLogger()
    {
        static Logger sLogger("[GLEAM]");
        return sLogger;
    }
    
    static const Logger& GetClientLogger()
    {
        static Logger sLogger("[APP]");
        return sLogger;
    }
    
    TString Log(Level lvl, const TStringView frmt, ...) const;
    
private:
    
    Logger(const TStringView name);
    
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
#define GLEAM_CORE_CRITICAL(...) ::Gleam::Logger::GetCoreLogger().Log(::Gleam::Logger::Level::Critical, __VA_ARGS__)

// Client log macros
#define GLEAM_TRACE(...) ::Gleam::Logger::GetClientLogger().Log(::Gleam::Logger::Level::Trace, __VA_ARGS__)
#define GLEAM_INFO(...) ::Gleam::Logger::GetClientLogger().Log(::Gleam::Logger::Level::Info, __VA_ARGS__)
#define GLEAM_WARN(...) ::Gleam::Logger::GetClientLogger().Log(::Gleam::Logger::Level::Warn, __VA_ARGS__)
#define GLEAM_ERROR(...) ::Gleam::Logger::GetClientLogger().Log(::Gleam::Logger::Level::Error, __VA_ARGS__)
#define GLEAM_CRITICAL(...) ::Gleam::Logger::GetClientLogger().Log(::Gleam::Logger::Level::Critical, __VA_ARGS__)

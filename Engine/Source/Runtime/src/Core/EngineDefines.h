#pragma once
#include "PlatformTargetDefines.h"

#ifdef GDEBUG
	#if defined(PLATFORM_WINDOWS)
		#define DEBUGBREAK() __debugbreak()

	#elif defined(PLATFORM_LINUX) || defined(PLATFORM_MACOS)
		#include <signal.h>
		#define DEBUGBREAK() raise(SIGTRAP)

	#else
		#error "Platform doesn't support debugbreak yet!"
	#endif
	#define ENABLE_ASSERTS
#else
	#define DEBUGBREAK()
#endif

#define EXPAND(x) x
#define STRINGIFY(x) #x
#define BIT(x) (1 << x)

#ifdef ENABLE_ASSERTS
	#define INTERNAL_ASSERT_IMPL(type, check, msg, ...) { if(!(check)) { GLEAM##type##ERROR(msg, __VA_ARGS__); DEBUGBREAK(); } }
	#define INTERNAL_ASSERT_WITH_MSG(type, check, ...) INTERNAL_ASSERT_IMPL(type, check, "Assertion failed: {0}", __VA_ARGS__)
	#define INTERNAL_ASSERT_NO_MSG(type, check) INTERNAL_ASSERT_IMPL(type, check, "Assertion '{0}' failed at {1}:{2}", STRINGIFY(check), std::filesystem::path(__FILE__).filename().string(), __LINE__)

	#define INTERNAL_ASSERT_GET_MACRO_NAME(arg1, arg2, macro, ...) macro
	#define INTERNAL_ASSERT_GET_MACRO(...) EXPAND( INTERNAL_ASSERT_GET_MACRO_NAME(__VA_ARGS__, INTERNAL_ASSERT_WITH_MSG, INTERNAL_ASSERT_NO_MSG) )

	#define STATIC_ASSERT(check, msg) static_assert(check, msg)
	#define DEBUG_ASSERT(...) EXPAND( INTERNAL_ASSERT_GET_MACRO(__VA_ARGS__)(_, __VA_ARGS__) )
	#define CORE_DEBUG_ASSERT(...) EXPAND( INTERNAL_ASSERT_GET_MACRO(__VA_ARGS__)(_CORE_, __VA_ARGS__) )
#else
	#define STATIC_ASSERT(check, msg)
	#define DEBUG_ASSERT(...)
	#define CORE_DEBUG_ASSERT(...)
#endif

#define MATH_INLINE [[nodiscard]] inline
#define PASS_BY_VALUE typename = std::enable_if_t<(sizeof(T) <= sizeof(int))>
#define PASS_BY_REFERENCE typename = std::enable_if_t<(sizeof(T) > sizeof(int))>
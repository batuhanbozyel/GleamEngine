#pragma once
#include "PlatformTargetDefines.h"

#ifdef GDEBUG
	#if defined(PLATFORM_WINDOWS)
		#define DEBUGBREAK() (__nop(), __debugbreak())

	#elif defined(PLATFORM_LINUX) || defined(PLATFORM_MACOS)
		#include <signal.h>
		#define DEBUGBREAK() (raise(SIGTRAP))

	#else
		#error "Platform doesn't support debugbreak yet!"
	#endif
	#define ENABLE_ASSERTS
	#define FORCE_INLINE inline
#else
	#define DEBUGBREAK()
	#ifdef PLATFORM_WINDOWS
    	#define FORCE_INLINE __forceinline
    #else
    	#define FORCE_INLINE inline __attribute__ ((always_inline))
    #endif
#endif

#ifdef ENABLE_ASSERTS
#include <filesystem>
	#define GLEAM_ASSERT(x, ...) if (!(x)) { GLEAM_CORE_ERROR("Assertion failed at {0}:{1}", std::filesystem::path(__FILE__).filename().string(), __LINE__); DEBUGBREAK(); }
#else
	#define GLEAM_ASSERT(...)
#endif

#define BIT(x) (1 << (x))
#define NO_DISCARD [[nodiscard]]

#ifdef __OBJC__
#define TO_CPP_STRING(x) std::string([(x) UTF8String])
#define TO_NSSTRING(x) ([NSString stringWithCString:(x) encoding:NSUTF8StringEncoding])
#endif

#define GLEAM_STRINGIFY(x) #x
#define GLEAM_CONCAT(x, y) x##y
#define GLEAM_EXPAND(X) X

#define GLEAM_FOREACH_ARG_N(_1, _2, _3, _4, _5, _6, _7, _8, _9, N, ...) N
#define GLEAM_FOREACH_RSEQ_N() 9, 8, 7, 6, 5, 4, 3, 2, 1, 0

#define GLEAM_FOREACH(MACRO, ...) GLEAM_FOREACH_(GLEAM_FOREACH_NUM_ARGS(__VA_ARGS__), MACRO, __VA_ARGS__)
#define GLEAM_FOREACH_(N, M, ...) GLEAM_EXPAND(GLEAM_CONCAT(GLEAM_FOREACH_, N)(M, __VA_ARGS__))
#define GLEAM_FOREACH_NUM_ARGS(...) GLEAM_FOREACH_NUM_ARGS_(__VA_ARGS__, GLEAM_FOREACH_RSEQ_N())
#define GLEAM_FOREACH_NUM_ARGS_(...) GLEAM_EXPAND(GLEAM_FOREACH_ARG_N(__VA_ARGS__))

#define GLEAM_FOREACH_1(M, x, ...) M(x)
#define GLEAM_FOREACH_2(M, x, ...) M(x) GLEAM_EXPAND(GLEAM_FOREACH_1(M, __VA_ARGS__))
#define GLEAM_FOREACH_3(M, x, ...) M(x) GLEAM_EXPAND(GLEAM_FOREACH_2(M, __VA_ARGS__))
#define GLEAM_FOREACH_4(M, x, ...) M(x) GLEAM_EXPAND(GLEAM_FOREACH_3(M, __VA_ARGS__))
#define GLEAM_FOREACH_5(M, x, ...) M(x) GLEAM_EXPAND(GLEAM_FOREACH_4(M, __VA_ARGS__))
#define GLEAM_FOREACH_6(M, x, ...) M(x) GLEAM_EXPAND(GLEAM_FOREACH_5(M, __VA_ARGS__))
#define GLEAM_FOREACH_7(M, x, ...) M(x) GLEAM_EXPAND(GLEAM_FOREACH_6(M, __VA_ARGS__))
#define GLEAM_FOREACH_8(M, x, ...) M(x) GLEAM_EXPAND(GLEAM_FOREACH_7(M, __VA_ARGS__))
#define GLEAM_FOREACH_9(M, x, ...) M(x) GLEAM_EXPAND(GLEAM_FOREACH_8(M, __VA_ARGS__))


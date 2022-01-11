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

#define BIT(x) (1 << x)

#ifdef ENABLE_ASSERTS
	#define GLEAM_ASSERT(x, ...) if (!(x)) { GLEAM_CORE_ERROR("Assertion failed at {0}:{1}", std::filesystem::path(__FILE__).filename().string(), __LINE__); DEBUGBREAK(); }
#else
	#define GLEAM_ASSERT(...)
#endif

#define MATH_INLINE [[nodiscard]] inline
#define PASS_BY_VALUE typename = std::enable_if_t<(sizeof(T) <= sizeof(int))>
#define PASS_BY_REFERENCE typename = std::enable_if_t<(sizeof(T) > sizeof(int))>

#define GLEAM_ENGINE_MAJOR_VERSION 1
#define GLEAM_ENGINE_MINOR_VERSION 0
#define GLEAM_ENGINE_PATCH_VERSION 0
#define GLEAM_ENGINE_VERSION constexpr Gleam::Version(GLEAM_ENGINE_MAJOR_VERSION, GLEAM_ENGINE_MINOR_VERSION, GLEAM_ENGINE_PATCH_VERSION)

#ifdef USE_VULKAN_RENDERER
#define VULKAN_API_VERSION VK_API_VERSION_1_0
#endif

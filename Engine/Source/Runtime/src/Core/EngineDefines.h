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
#else
	#define DEBUGBREAK()
#endif

#define BIT(x) (1 << (x))

#ifdef ENABLE_ASSERTS
	#define GLEAM_ASSERT(x, ...) if (!(x)) { GLEAM_CORE_ERROR("Assertion failed at {0}:{1}", std::filesystem::path(__FILE__).filename().string(), __LINE__); DEBUGBREAK(); }
#else
	#define GLEAM_ASSERT(...)
#endif

#ifdef USE_VULKAN_RENDERER
#define VULKAN_API_VERSION VK_API_VERSION_1_1
using NativeGraphicsHandle = void*;
using DispatchSemaphore = void*;
#else
#include <objc/objc-runtime.h>
using NativeGraphicsHandle = id;
using DispatchSemaphore = dispatch_semaphore_t;
#endif

#ifdef GDEBUG
#define FORCE_INLINE inline
#else
    #ifdef PLATFORM_WINDOWS
    #define FORCE_INLINE __forceinline
    #else
    #define FORCE_INLINE inline __attribute__ ((always_inline))
    #endif
#endif

#define NO_DISCARD [[nodiscard]]
#define PASS_BY_VALUE typename = std::enable_if_t<(sizeof(T) <= sizeof(int))>
#define PASS_BY_REFERENCE typename = std::enable_if_t<(sizeof(T) > sizeof(int))>

template<typename T, typename P>
constexpr inline T As(P p)
{
	return reinterpret_cast<T>(p);
}

#define ApplicationInstance Gleam::Application::GetInstance()

#define GLEAM_ENGINE_MAJOR_VERSION 1
#define GLEAM_ENGINE_MINOR_VERSION 0
#define GLEAM_ENGINE_PATCH_VERSION 0
#define GLEAM_ENGINE_VERSION constexpr Gleam::Version(GLEAM_ENGINE_MAJOR_VERSION, GLEAM_ENGINE_MINOR_VERSION, GLEAM_ENGINE_PATCH_VERSION)

#ifdef __OBJC__
#define OBJC_CLASS(name) @class name
#else
#define OBJC_CLASS(name) typedef struct objc_object name
#endif

#define GLEAM_NONCOPYABLE(TypeName) \
    TypeName(const TypeName&) = delete; \
    TypeName& operator=(const TypeName&) = delete; \
    TypeName& operator=(TypeName&&) = delete

#define COMPONENT_BODY() using Component::Component

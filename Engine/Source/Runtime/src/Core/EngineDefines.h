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
#include "IO/Filesystem.h"
	#define GLEAM_ASSERT(x, ...) if (!(x)) { GLEAM_CORE_ERROR("Assertion failed at {0}:{1}", Gleam::Filesystem::path(__FILE__).filename().string(), __LINE__); DEBUGBREAK(); }
#else
	#define GLEAM_ASSERT(...)
#endif

#if defined(USE_DIRECTX_RENDERER)
#include <d3d12.h>
using NativeGraphicsHandle = void*;
using NativeGraphicsResourceView = D3D12_CPU_DESCRIPTOR_HANDLE;
using DispatchSemaphore = NativeGraphicsHandle;
#else
#include <objc/objc-runtime.h>
#include <dispatch/dispatch.h>
using NativeGraphicsHandle = id;
using NativeGraphicsResourceView = id;
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

namespace Gleam {

template<typename T, typename P>
constexpr inline T As(P p)
{
	return reinterpret_cast<T>(p);
}

} // namespace Gleam


#define GameInstance Gleam::Application::GetInstance()

#define GLEAM_ENGINE_MAJOR_VERSION 1
#define GLEAM_ENGINE_MINOR_VERSION 0
#define GLEAM_ENGINE_PATCH_VERSION 0
#define GLEAM_ENGINE_VERSION constexpr Gleam::Version(GLEAM_ENGINE_MAJOR_VERSION, GLEAM_ENGINE_MINOR_VERSION, GLEAM_ENGINE_PATCH_VERSION)

#ifdef __OBJC__
#define TO_CPP_STRING(x) std::string([(x) UTF8String])
#define TO_NSSTRING(x) ([NSString stringWithCString:(x) encoding:NSUTF8StringEncoding])
#endif

#define GLEAM_NONCOPYABLE(TypeName) \
    TypeName(const TypeName&) = delete; \
    TypeName& operator=(const TypeName&) = delete; \
	TypeName(TypeName&&) = delete; \
    TypeName& operator=(TypeName&&) = delete

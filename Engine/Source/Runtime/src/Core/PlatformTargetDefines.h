#pragma once

#ifdef _WIN32
	#ifdef _WIN64
	#define PLATFORM_WINDOWS
	#else
	#error Gleam Engine does not support x86 architechture!
	#endif

#elif defined(__APPLE__) || defined(__MACH__)
#include <TargetConditionals.h>
	#if TARGET_IPHONE_SIMULATOR == 1
	#elif TARGET_OS_IPHONE == 1
	#define PLATFORM_IOS
	#elif TARGET_OS_MAC == 1
	#define PLATFORM_MACOS
	#else
	#error Unknown Apple platform!
	#endif

#elif defined(__ANDROID__)
#define PLATFORM_ANDROID

#elif defined(__linux__)
#define PLATFORM_LINUX

#else
#error Unknown platform!
#endif

#if defined(PLATFORM_MACOS) || defined(PLATFORM_IOS)
#define USE_METAL_RENDERER
#else
#define USE_VULKAN_RENDERER
	#ifdef PLATFORM_WINDOWS
	#define VK_USE_PLATFORM_WIN32_KHR

	#elif defined(PLATFORM_ANDROID)
	#define VK_USE_PLATFORM_ANDROID_KHR

	#elif defined(PLATFORM_LINUX)
	#define VK_USE_PLATFORM_XCB_KHR
	#define VK_USE_PLATFORM_XLIB_KHR
	#define VK_USE_PLATFORM_WAYLAND_KHR
	#endif
#endif
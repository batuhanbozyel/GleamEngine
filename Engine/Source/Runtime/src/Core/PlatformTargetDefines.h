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
#define USE_DIRECTX_RENDERER
#endif

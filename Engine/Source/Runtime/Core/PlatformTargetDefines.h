#pragma once

#ifdef GLEAM_DYNAMIC
#ifdef _MSC_VER
	#ifndef GLEAM_API
		#ifdef GLEAM_EXPORT
			#define GLEAM_API __declspec(dllexport)
		#else
			#define GLEAM_API __declspec(dllimport)
		#endif
	#endif

	#ifndef GLEAM_NO_EXPORT
		#define GLEAM_NO_EXPORT
	#endif
#else
	#ifndef GLEAM_API
		#define GLEAM_API __attribute__((visibility("default")))
	#endif
	#ifndef GLEAM_NO_EXPORT
		#define GLEAM_NO_EXPORT __attribute__((visibility("hidden")))
	#endif
#endif
#else
	#define GLEAM_API
	#define GLEAM_NO_EXPORT
#endif

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
#pragma once
#include "Macro.h"

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

#define GameInstance Gleam::Application::GetInstance()
#define GLEAM_ENGINE_MAJOR_VERSION 1
#define GLEAM_ENGINE_MINOR_VERSION 0
#define GLEAM_ENGINE_PATCH_VERSION 0
#define GLEAM_ENGINE_VERSION constexpr Gleam::Version(GLEAM_ENGINE_MAJOR_VERSION, GLEAM_ENGINE_MINOR_VERSION, GLEAM_ENGINE_PATCH_VERSION)

#define GLEAM_NONCOPYABLE(TypeName) \
    TypeName(const TypeName&) = delete; \
    TypeName& operator=(const TypeName&) = delete; \
	TypeName(TypeName&&) = delete; \
    TypeName& operator=(TypeName&&) = delete
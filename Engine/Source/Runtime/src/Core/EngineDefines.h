#pragma once
#include "Macro.h"

#if defined(USE_DIRECTX_RENDERER)
#include <d3d12.h>
using NativeGraphicsHandle = void*;
using RenderTargetView = D3D12_CPU_DESCRIPTOR_HANDLE;
#else
#import <Metal/MTLTypes.h>
#import <objc/objc-runtime.h>
using NativeGraphicsHandle = id;
using RenderTargetView = MTLResourceID;
#endif

#define GLEAM_ENGINE_MAJOR_VERSION 1
#define GLEAM_ENGINE_MINOR_VERSION 0
#define GLEAM_ENGINE_PATCH_VERSION 0
#define GLEAM_ENGINE_VERSION constexpr Gleam::Version(GLEAM_ENGINE_MAJOR_VERSION, GLEAM_ENGINE_MINOR_VERSION, GLEAM_ENGINE_PATCH_VERSION)

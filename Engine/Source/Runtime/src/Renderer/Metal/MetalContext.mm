//
//  MetalContext.m
//  GleamEngine
//
//  Created by Batuhan Bozyel on 20.03.2023.
//

#include "gpch.h"

#ifdef USE_METAL_RENDERER
#include "Renderer/RendererContext.h"
#include "MetalDevice.h"

using namespace Gleam;

void RendererContext::WaitDeviceIdle() const
{
    
}

void RendererContext::ConfigureBackend()
{
    MetalDevice::Init();
}

void RendererContext::DestroyBackend()
{
    MetalDevice::Destroy();
}

void RendererContext::Configure(const RendererConfig& config)
{
    MetalDevice::GetSwapchain().Configure(config);
}

void RendererContext::AddPooledObject(std::any object, std::function<void(std::any)> deallocator)
{
    MetalDevice::GetSwapchain().AddPooledObject(object, deallocator);
}

uint32_t RendererContext::GetFrameIndex() const
{
    return MetalDevice::GetSwapchain().GetFrameIndex();
}

uint32_t RendererContext::GetFramesInFlight() const
{
    return MetalDevice::GetSwapchain().GetFramesInFlight();
}

const Gleam::Size& RendererContext::GetDrawableSize() const
{
    return MetalDevice::GetSwapchain().GetSize();
}

#endif

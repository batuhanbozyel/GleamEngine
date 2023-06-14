//
//  MetalContext.m
//  GleamEngine
//
//  Created by Batuhan Bozyel on 20.03.2023.
//

#include "gpch.h"

#ifdef USE_METAL_RENDERER
#include "Renderer/RendererContext.h"
#include "Renderer/RenderPipeline.h"
#include "MetalDevice.h"

using namespace Gleam;

void RendererContext::ConfigureBackend(const RendererConfig& config)
{
    mConfiguration = config;
    MetalDevice::Init(mConfiguration);
    RenderPipeline::mRendererContext = this;
    mCommandBuffer = CreateScope<CommandBuffer>();
}

void RendererContext::DestroyBackend()
{
    mCommandBuffer.reset();
    MetalDevice::Destroy();
}

void RendererContext::SetConfiguration(const RendererConfig& config)
{
    mConfiguration = config;
    MetalDevice::GetSwapchain().Configure(config);
}

const Gleam::Size& RendererContext::GetDrawableSize() const
{
    return MetalDevice::GetSwapchain().GetSize();
}

#endif

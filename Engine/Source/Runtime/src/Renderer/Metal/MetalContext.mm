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

void RendererContext::ConfigureBackend(const TString& appName, const Version& appVersion, const RendererConfig& config)
{
    MetalDevice::Init(appName, config);
    mConfiguration = config;
    mConfiguration.format = MetalDevice::GetSwapchain().GetFormat();
    RenderPipeline::mRendererContext = this;
}

void RendererContext::DestroyBackend()
{
    MetalDevice::Destroy();
}

RefCounted<RenderTexture> RendererContext::GetSwapchainTarget() const
{
    TextureDescriptor descriptor;
    descriptor.sampleCount = mConfiguration.sampleCount;
    descriptor.size = MetalDevice::GetSwapchain().GetSize();
    descriptor.format = MetalDevice::GetSwapchain().GetFormat();
    
    id<CAMetalDrawable> drawable = MetalDevice::GetSwapchain().AcquireNextDrawable();
    return CreateRef<RenderTexture>(drawable.texture, drawable.texture, descriptor);
}

#endif

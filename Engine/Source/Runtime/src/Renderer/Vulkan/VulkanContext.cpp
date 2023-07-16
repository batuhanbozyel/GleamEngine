//
//  VulkanContext.cpp
//  GleamEngine
//
//  Created by Batuhan Bozyel on 20.03.2023.
//

#include "gpch.h"

#ifdef USE_VULKAN_RENDERER
#include "Renderer/RendererContext.h"
#include "VulkanDevice.h"

using namespace Gleam;

void RendererContext::ConfigureBackend()
{
    VulkanDevice::Init();
}

void RendererContext::DestroyBackend()
{
    VulkanDevice::Destroy();
}

void RendererContext::Configure(const RendererConfig& config) const
{
    VulkanDevice::GetSwapchain().Configure(config);
}

const Gleam::Size& RendererContext::GetDrawableSize() const
{
    return VulkanDevice::GetSwapchain().GetSize();
}

#endif
